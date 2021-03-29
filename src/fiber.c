/*
  +--------------------------------------------------------------------+
  | ext-fiber                                                          |
  +--------------------------------------------------------------------+
  | Redistribution and use in source and binary forms, with or without |
  | modification, are permitted provided that the conditions mentioned |
  | in the accompanying LICENSE file are met.                          |
  +--------------------------------------------------------------------+
  | Authors: Aaron Piotrowski <aaron@trowski.com>                      |
  |          Martin Schröder <m.schroeder2007@gmail.com>               |
  +--------------------------------------------------------------------+
*/

#include "php.h"
#include "zend.h"
#include "zend_API.h"
#include "zend_vm.h"
#include "zend_interfaces.h"
#include "zend_exceptions.h"
#include "zend_closures.h"
#include "zend_observer.h"
#include "zend_builtin_functions.h"

#include "php_fiber.h"
#include "fiber.h"

PHP_FIBER_API zend_class_entry *zend_ce_fiber;

static zend_class_entry *zend_ce_reflection_fiber;

static zend_class_entry *zend_ce_fiber_error;
static zend_class_entry *zend_ce_fiber_exit;

static zend_object_handlers zend_fiber_handlers;
static zend_object_handlers zend_reflection_fiber_handlers;

static zend_object *zend_fiber_object_create(zend_class_entry *ce);
static void zend_fiber_object_destroy(zend_object *object);

zend_llist zend_fiber_observers_list;

#define ZEND_FIBER_BACKUP_EG(stack, stack_page_size, execute_data, error_reporting, trace_num) do { \
	stack = EG(vm_stack); \
	stack->top = EG(vm_stack_top); \
	stack->end = EG(vm_stack_end); \
	stack_page_size = EG(vm_stack_page_size); \
	execute_data = EG(current_execute_data); \
	error_reporting = EG(error_reporting); \
	trace_num = EG(jit_trace_num); \
} while (0)

#define ZEND_FIBER_RESTORE_EG(stack, stack_page_size, execute_data, error_reporting, trace_num) do { \
	EG(vm_stack) = stack; \
	EG(vm_stack_top) = stack->top; \
	EG(vm_stack_end) = stack->end; \
	EG(vm_stack_page_size) = stack_page_size; \
	EG(current_execute_data) = execute_data; \
	EG(error_reporting) = error_reporting; \
	EG(jit_trace_num) = trace_num; \
} while (0)



PHP_FIBER_API zend_fiber *zend_get_current_fiber(void)
{
	return FIBER_G(current_fiber);
}


zend_always_inline PHP_FIBER_API zend_bool zend_is_fiber_exit(const zend_object *exception)
{
	ZEND_ASSERT(exception && "No exception object provided");

	return exception->ce == zend_ce_fiber_exit;
}


PHP_FIBER_API void zend_observer_fiber_switch_register(zend_observer_fiber_switch_handler handler)
{
	zend_llist_add_element(&zend_fiber_observers_list, &handler);
}


zend_always_inline static void zend_observer_fiber_switch_notify(zend_fiber *from, zend_fiber *to)
{
	zend_llist_element *element;
	zend_observer_fiber_switch_handler callback;

	for (element = zend_fiber_observers_list.head; element; element = element->next) {
		callback = *(zend_observer_fiber_switch_handler *) element->data;
		callback(from, to);
	}
}


static zend_bool zend_fiber_switch_to(zend_fiber *fiber)
{
	zend_fiber *previous;
	zend_vm_stack stack;
	size_t stack_page_size;
	zend_execute_data *execute_data;
	int error_reporting;
	uint32_t jit_trace_num;
	zend_bool result;

	previous = FIBER_G(current_fiber);

	FIBER_G(current_fiber) = fiber;

	zend_observer_fiber_switch_notify(previous, fiber);

	ZEND_FIBER_BACKUP_EG(stack, stack_page_size, execute_data, error_reporting, jit_trace_num);

	result = zend_fiber_switch_context(fiber->context);

	FIBER_G(current_fiber) = previous;

	ZEND_FIBER_RESTORE_EG(stack, stack_page_size, execute_data, error_reporting, jit_trace_num);

	zend_observer_fiber_switch_notify(fiber, previous);

	return result;
}


static zend_bool zend_fiber_suspend(zend_fiber *fiber)
{
	zend_vm_stack stack;
	size_t stack_page_size;
	zend_execute_data *execute_data;
	uint32_t jit_trace_num;
	int error_reporting;
	zend_bool result;

	ZEND_FIBER_BACKUP_EG(stack, stack_page_size, execute_data, error_reporting, jit_trace_num);

	result = zend_fiber_suspend_context(fiber->context);

	ZEND_FIBER_RESTORE_EG(stack, stack_page_size, execute_data, error_reporting, jit_trace_num);

	return result;
}


static zend_always_inline zend_vm_stack zend_fiber_vm_stack_alloc(size_t size)
{
	zend_vm_stack page = emalloc(size);

	page->top = ZEND_VM_STACK_ELEMENTS(page);
	page->end = (zval *) ((char *) page + size);
	page->prev = NULL;

	return page;
}

static void zend_fiber_execute(zend_fiber *fiber)
{
	zval retval;

	fiber->status = ZEND_FIBER_STATUS_RUNNING;
	fiber->fci.retval = &retval;

	zend_call_function(&fiber->fci, &fiber->fci_cache);

	if (EG(exception)) {
		if (fiber->status == ZEND_FIBER_STATUS_SHUTDOWN) {
			zend_clear_exception();
		} else {
			fiber->status = ZEND_FIBER_STATUS_THREW;
		}
	} else {
		fiber->status = ZEND_FIBER_STATUS_RETURNED;
	}

	ZVAL_COPY(&fiber->value, &retval);
	zval_ptr_dtor(&retval);

	GC_DELREF(&fiber->std);
}


ZEND_NORETURN static void zend_fiber_run(void)
{
	zend_fiber *fiber = FIBER_G(current_fiber);
	ZEND_ASSERT(fiber != NULL);

	zend_long error_reporting = INI_INT("error_reporting");
	if (!error_reporting && !INI_STR("error_reporting")) {
		error_reporting = E_ALL;
	}

	zend_vm_stack stack = zend_fiber_vm_stack_alloc(ZEND_FIBER_VM_STACK_SIZE);
	EG(vm_stack) = stack;
	EG(vm_stack_top) = (zval *) stack->top + ZEND_CALL_FRAME_SLOT;
	EG(vm_stack_end) = stack->end;
	EG(vm_stack_page_size) = ZEND_FIBER_VM_STACK_SIZE;

	fiber->execute_data = (zend_execute_data *) stack->top;

	ZEND_SECURE_ZERO(fiber->execute_data, sizeof(zend_execute_data));

	EG(current_execute_data) = fiber->execute_data;
	EG(jit_trace_num) = 0;
	EG(error_reporting) = error_reporting;

	zend_fiber_execute(fiber);

	zend_vm_stack_destroy();
	fiber->execute_data = NULL;

	zend_fiber_suspend_context(fiber->context);

	abort();
}


static void zend_fiber_free(zend_fiber *fiber)
{
	if (fiber->status == ZEND_FIBER_STATUS_SUSPENDED) {
		fiber->status = ZEND_FIBER_STATUS_SHUTDOWN;
		zend_fiber_switch_to(fiber);
	}

	zend_fiber_destroy_context(fiber->context);

	zend_object_std_dtor(&fiber->std);
}


static zend_object *zend_fiber_object_create(zend_class_entry *ce)
{
	zend_fiber *fiber;

	fiber = emalloc(sizeof(zend_fiber));
	ZEND_SECURE_ZERO(fiber, sizeof(zend_fiber));

	fiber->id = FIBER_G(id)++;

	zend_object_std_init(&fiber->std, ce);
	fiber->std.handlers = &zend_fiber_handlers;

	ZVAL_UNDEF(&fiber->value);

	zend_hash_index_add_ptr(&FIBER_G(fibers), fiber->std.handle, fiber);

	return &fiber->std;
}


static void zend_fiber_object_destroy(zend_object *object)
{
	zend_fiber *fiber = (zend_fiber *) object;

	zval_ptr_dtor(&fiber->value);

	zend_hash_index_del(&FIBER_G(fibers), fiber->std.handle);

	if (fiber->status == ZEND_FIBER_STATUS_INIT) {
		// Fiber was never started, so we need to release the reference to the callback.
		zval_ptr_dtor(&fiber->fci.function_name);
	}

	zend_fiber_free(fiber);
}


static int zend_fiber_catch_handler(zend_execute_data *execute_data)
{
	if (UNEXPECTED(EG(exception) && zend_is_fiber_exit(EG(exception)))) {
		zend_rethrow_exception(execute_data);
		return ZEND_USER_OPCODE_CONTINUE;
	}

	if (FIBER_G(catch_handler)) {
		return FIBER_G(catch_handler)(execute_data);
	}

	return ZEND_USER_OPCODE_DISPATCH;
}


static void zend_fiber_clean_shutdown(void)
{
	zend_fiber *fiber;

	ZEND_HASH_REVERSE_FOREACH_PTR(&FIBER_G(fibers), fiber) {
		if (fiber->status == ZEND_FIBER_STATUS_SUSPENDED) {
			fiber->status = ZEND_FIBER_STATUS_SHUTDOWN;
			GC_ADDREF(&fiber->std);
			zend_fiber_switch_to(fiber);
		}
	} ZEND_HASH_FOREACH_END();
}


static void zend_fiber_shutdown_cleanup(void)
{
	zend_fiber *fiber;

	ZEND_HASH_REVERSE_FOREACH_PTR(&FIBER_G(fibers), fiber) {
		if (fiber->status == ZEND_FIBER_STATUS_SUSPENDED) {
			fiber->status = ZEND_FIBER_STATUS_SHUTDOWN;
			GC_ADDREF(&fiber->std);
		}
	} ZEND_HASH_FOREACH_END();
}


void zend_fiber_error_observer(int type, const char *filename, uint32_t line, zend_string *message)
{
	if (!(type & E_FATAL_ERRORS)) {
		return; // Non-fatal error, nothing to do.
	}

	if (FIBER_G(shutdown)) {
		return; // Already shut down, nothing to do.
	}

	// Fatal error, mark as shutdown.
	FIBER_G(shutdown) = 1;

	if (type & E_DONT_BAIL) {
		// Uncaught exception, do a clean shutdown now.
		zend_fiber_clean_shutdown();
	}
}


static zend_object *zend_reflection_fiber_object_create(zend_class_entry *ce)
{
	zend_fiber_reflection *reflection;

	reflection = emalloc(sizeof(zend_fiber_reflection));
	ZEND_SECURE_ZERO(reflection, sizeof(zend_fiber_reflection));

	zend_object_std_init(&reflection->std, ce);
	reflection->std.handlers = &zend_reflection_fiber_handlers;

	return &reflection->std;
}


static void zend_reflection_fiber_object_destroy(zend_object *object)
{
	zend_fiber_reflection *reflection = (zend_fiber_reflection *) object;

	if (reflection->fiber != NULL) {
		GC_DELREF(&reflection->fiber->std);
	}

	zend_object_std_dtor(&reflection->std);
}


/* {{{ proto Fiber::__construct(callable $callback) */
ZEND_METHOD(Fiber, __construct)
{
	zend_fiber *fiber = (zend_fiber *) Z_OBJ_P(getThis());

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_FUNC_EX(fiber->fci, fiber->fci_cache, 0, 0)
	ZEND_PARSE_PARAMETERS_END();

	// Keep a reference to closures or callable objects until the fiber is started.
	Z_TRY_ADDREF(fiber->fci.function_name);
}
/* }}} */


/* {{{ proto void Fiber::start(mixed ...$args) */
ZEND_METHOD(Fiber, start)
{
	zend_fiber *fiber;
	zval *params;
	uint32_t param_count;

	if (UNEXPECTED(FIBER_G(shutdown))) {
		zend_throw_error(zend_ce_fiber_error, "Cannot start a fiber during shutdown");
		return;
	}

	fiber = (zend_fiber *) Z_OBJ_P(getThis());

	if (fiber->status != ZEND_FIBER_STATUS_INIT) {
		zend_throw_error(zend_ce_fiber_error, "Cannot start a fiber that has already been started");
		return;
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 0, -1)
		Z_PARAM_VARIADIC('+', params, param_count)
	ZEND_PARSE_PARAMETERS_END();

	fiber->fci.params = params;
	fiber->fci.param_count = param_count;

	fiber->context = zend_fiber_create_context(zend_fiber_run, FIBER_G(stack_size));

	if (fiber->context == NULL) {
		zend_throw_error(zend_ce_fiber_exit, "Failed to create native fiber context");
		return;
	}

	fiber->status = ZEND_FIBER_STATUS_RUNNING;

	GC_ADDREF(&fiber->std);

	if (!zend_fiber_switch_to(fiber)) {
		fiber->status = ZEND_FIBER_STATUS_INIT;
		GC_DELREF(&fiber->std);
		zend_throw_error(zend_ce_fiber_exit, "Failed switching to fiber");
		return;
	}

	zval_ptr_dtor(&fiber->fci.function_name);

	if (fiber->status & ZEND_FIBER_STATUS_FINISHED) {
		RETURN_NULL();
	}

	RETVAL_COPY_VALUE(&fiber->value);
	ZVAL_UNDEF(&fiber->value);
}
/* }}} */


/* {{{ proto mixed Fiber::suspend(mixed $value) */
ZEND_METHOD(Fiber, suspend)
{
	zend_fiber *fiber;
	zval *error, *value = NULL;

	if (UNEXPECTED(FIBER_G(shutdown))) {
		zend_throw_error(zend_ce_fiber_error, "Cannot suspend during shutdown");
		return;
	}

	fiber = FIBER_G(current_fiber);

	if (UNEXPECTED(fiber == NULL)) {
		zend_throw_error(zend_ce_fiber_error, "Cannot suspend outside of a fiber");
		return;
	}

	if (UNEXPECTED(fiber->status != ZEND_FIBER_STATUS_RUNNING)) {
		if (fiber->status == ZEND_FIBER_STATUS_SHUTDOWN) {
			zend_throw_error(zend_ce_fiber_error, "Cannot suspend in a force closed fiber");
		} else {
			zend_throw_error(zend_ce_fiber_error, "Cannot suspend in a fiber that is not running");
		}
		return;
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 0, 1)
		Z_PARAM_OPTIONAL
		Z_PARAM_ZVAL(value);
	ZEND_PARSE_PARAMETERS_END();

	if (value != NULL) {
		ZVAL_COPY(&fiber->value, value);
	} else {
		ZVAL_NULL(&fiber->value);
	}

	fiber->execute_data = execute_data;
	fiber->status = ZEND_FIBER_STATUS_SUSPENDED;

	GC_DELREF(&fiber->std);

	if (!zend_fiber_suspend(fiber)) {
		fiber->status = ZEND_FIBER_STATUS_RUNNING;
		zend_throw_error(zend_ce_fiber_exit, "Failed suspending fiber");
		return;
	}

	if (fiber->status == ZEND_FIBER_STATUS_SHUTDOWN) {
		// This occurs on exit if the fiber never resumed, it has been GC'ed, so do not add a ref.
		zend_throw_error(zend_ce_fiber_exit, "Fiber destroyed");
		return;
	}

	fiber->status = ZEND_FIBER_STATUS_RUNNING;
	GC_ADDREF(&fiber->std);

	if (fiber->error == NULL) {
		RETVAL_COPY_VALUE(&fiber->value);
		ZVAL_UNDEF(&fiber->value);
		return;
	}

	error = fiber->error;
	fiber->error = NULL;

	execute_data->opline--;
	zend_throw_exception_object(error);
	execute_data->opline++;
}
/* }}} */


/* {{{ proto void Fiber::resume(mixed $value = null) */
ZEND_METHOD(Fiber, resume)
{
	zend_fiber *fiber;
	zval *value = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 0, 1)
		Z_PARAM_OPTIONAL
		Z_PARAM_ZVAL(value);
	ZEND_PARSE_PARAMETERS_END();

	fiber = (zend_fiber *) Z_OBJ_P(getThis());

	if (UNEXPECTED(fiber->status != ZEND_FIBER_STATUS_SUSPENDED)) {
		zend_throw_error(zend_ce_fiber_error, "Cannot resume a fiber that is not suspended");
		return;
	}

	if (value != NULL) {
		ZVAL_COPY(&fiber->value, value);
	} else {
		ZVAL_NULL(&fiber->value);
	}

	fiber->status = ZEND_FIBER_STATUS_RUNNING;

	if (!zend_fiber_switch_to(fiber)) {
		fiber->status = ZEND_FIBER_STATUS_SUSPENDED;
		zend_throw_error(zend_ce_fiber_exit, "Failed resuming fiber");
		return;
	}

	if (fiber->status & ZEND_FIBER_STATUS_FINISHED) {
		RETURN_NULL();
	}

	RETVAL_COPY_VALUE(&fiber->value);
	ZVAL_UNDEF(&fiber->value);
}
/* }}} */


/* {{{ proto void Fiber::throw(Throwable $exception) */
ZEND_METHOD(Fiber, throw)
{
	zend_fiber *fiber;
	zval *exception;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS_EX(exception, zend_ce_throwable, 0, 0)
	ZEND_PARSE_PARAMETERS_END();

	fiber = (zend_fiber *) Z_OBJ_P(getThis());

	if (UNEXPECTED(fiber->status != ZEND_FIBER_STATUS_SUSPENDED)) {
		zend_throw_error(zend_ce_fiber_error, "Cannot resume a fiber that is not suspended");
		return;
	}

	Z_ADDREF_P(exception);
	fiber->error = exception;

	fiber->status = ZEND_FIBER_STATUS_RUNNING;

	if (!zend_fiber_switch_to(fiber)) {
		fiber->status = ZEND_FIBER_STATUS_SUSPENDED;
		zend_throw_error(zend_ce_fiber_exit, "Failed resuming fiber");
		return;
	}

	if (fiber->status & ZEND_FIBER_STATUS_FINISHED) {
		RETURN_NULL();
	}

	RETVAL_COPY_VALUE(&fiber->value);
	ZVAL_UNDEF(&fiber->value);
}
/* }}} */


/* {{{ proto bool Fiber::isStarted() */
ZEND_METHOD(Fiber, isStarted)
{
	zend_fiber *fiber;

	ZEND_PARSE_PARAMETERS_NONE();

	fiber = (zend_fiber *) Z_OBJ_P(getThis());

	RETURN_BOOL(fiber->status != ZEND_FIBER_STATUS_INIT);
}
/* }}} */


/* {{{ proto bool Fiber::isSuspended() */
ZEND_METHOD(Fiber, isSuspended)
{
	zend_fiber *fiber;

	ZEND_PARSE_PARAMETERS_NONE();

	fiber = (zend_fiber *) Z_OBJ_P(getThis());

	RETURN_BOOL(fiber->status == ZEND_FIBER_STATUS_SUSPENDED);
}
/* }}} */


/* {{{ proto bool Fiber::isRunning() */
ZEND_METHOD(Fiber, isRunning)
{
	zend_fiber *fiber;

	ZEND_PARSE_PARAMETERS_NONE();

	fiber = (zend_fiber *) Z_OBJ_P(getThis());

	RETURN_BOOL(fiber->status == ZEND_FIBER_STATUS_RUNNING);
}
/* }}} */


/* {{{ proto bool Fiber::isTerminated() */
ZEND_METHOD(Fiber, isTerminated)
{
	zend_fiber *fiber;

	ZEND_PARSE_PARAMETERS_NONE();

	fiber = (zend_fiber *) Z_OBJ_P(getThis());

	RETURN_BOOL(fiber->status & ZEND_FIBER_STATUS_FINISHED);
}
/* }}} */


/* {{{ proto mixed Fiber::getReturn() */
ZEND_METHOD(Fiber, getReturn)
{
	zend_fiber *fiber;

	ZEND_PARSE_PARAMETERS_NONE();

	fiber = (zend_fiber *) Z_OBJ_P(getThis());

	if (fiber->status != ZEND_FIBER_STATUS_RETURNED) {
		const char *message;

		if (fiber->status == ZEND_FIBER_STATUS_INIT) {
			message = "The fiber has not been started";
		} else if (fiber->status == ZEND_FIBER_STATUS_THREW) {
			message = "The fiber threw an exception";
		} else {
			message = "The fiber has not returned";
		}

		zend_throw_error(zend_ce_fiber_error, "Cannot get fiber return value: %s", message);
		return;
	}

	RETURN_COPY(&fiber->value);
}
/* }}} */


/* {{{ proto Fiber|null Fiber::this() */
ZEND_METHOD(Fiber, this)
{
	zend_fiber *fiber;

	ZEND_PARSE_PARAMETERS_NONE();

	fiber = FIBER_G(current_fiber);

	if (fiber == NULL) {
		RETURN_NULL();
	}

	RETURN_OBJ_COPY(&fiber->std);
}
/* }}} */


/* {{{ proto FiberError::__construct(string $message) */
ZEND_METHOD(FiberError, __construct)
{
	zend_object *object = Z_OBJ_P(getThis());

	zend_throw_error(
		NULL,
		"The \"%s\" class is reserved for internal use and cannot be manually instantiated",
		object->ce->name->val
	);
}
/* }}} */


/* {{{ proto ReflectionFiber::fromFiber(Fiber $fiber) */
ZEND_METHOD(ReflectionFiber, __construct)
{
	zend_fiber_reflection *reflection;
	zval *object;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS_EX(object, zend_ce_fiber, 0, 0)
	ZEND_PARSE_PARAMETERS_END();

	reflection = (zend_fiber_reflection *) Z_OBJ_P(getThis());
	reflection->fiber = (zend_fiber *) Z_OBJ_P(object);

	GC_ADDREF(&reflection->fiber->std);
}
/* }}} */


/* {{{ proto Fiber ReflectionFiber::getFiber() */
ZEND_METHOD(ReflectionFiber, getFiber)
{
	zend_fiber_reflection *reflection;

	ZEND_PARSE_PARAMETERS_NONE();

	reflection = (zend_fiber_reflection *) Z_OBJ_P(getThis());

	RETURN_OBJ_COPY(&reflection->fiber->std);
}
/* }}} */


#define REFLECTION_CHECK_VALID_FIBER(fiber) do { \
		if (fiber == NULL || fiber->status == ZEND_FIBER_STATUS_INIT || fiber->status & ZEND_FIBER_STATUS_FINISHED) { \
			zend_throw_error(NULL, "Cannot fetch information from a fiber that has not been started or is terminated"); \
			return; \
		} \
	} while (0)


/* {{{ proto array ReflectionFiber::getTrace(int $options) */
ZEND_METHOD(ReflectionFiber, getTrace)
{
	zend_fiber_reflection *reflection;
	zend_long options = DEBUG_BACKTRACE_PROVIDE_OBJECT;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 0, 1)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(options);
	ZEND_PARSE_PARAMETERS_END();

	reflection = (zend_fiber_reflection *) Z_OBJ_P(getThis());

	REFLECTION_CHECK_VALID_FIBER(reflection->fiber);

	if (FIBER_G(current_fiber) != reflection->fiber) {
		// No need to replace current execute data if within the current fiber.
		EG(current_execute_data) = reflection->fiber->execute_data;
	}

	zend_fetch_debug_backtrace(return_value, 0, options, 0);

	EG(current_execute_data) = execute_data; // Restore original execute data.
}
/* }}} */


/* {{{ proto int ReflectionFiber::getExecutingLine() */
ZEND_METHOD(ReflectionFiber, getExecutingLine)
{
	zend_fiber_reflection *reflection;
	zend_execute_data *prev_execute_data;

	ZEND_PARSE_PARAMETERS_NONE();

	reflection = (zend_fiber_reflection *) Z_OBJ_P(getThis());

	REFLECTION_CHECK_VALID_FIBER(reflection->fiber);

	if (FIBER_G(current_fiber) == reflection->fiber) {
		prev_execute_data = execute_data->prev_execute_data;
	} else {
		prev_execute_data = reflection->fiber->execute_data->prev_execute_data;
	}

	RETURN_LONG(prev_execute_data->opline->lineno);
}
/* }}} */


/* {{{ proto string ReflectionFiber::getExecutingFile() */
ZEND_METHOD(ReflectionFiber, getExecutingFile)
{
	zend_fiber_reflection *reflection;
	zend_execute_data *prev_execute_data;

	ZEND_PARSE_PARAMETERS_NONE();

	reflection = (zend_fiber_reflection *) Z_OBJ_P(getThis());

	REFLECTION_CHECK_VALID_FIBER(reflection->fiber);

	if (FIBER_G(current_fiber) == reflection->fiber) {
		prev_execute_data = execute_data->prev_execute_data;
	} else {
		prev_execute_data = reflection->fiber->execute_data->prev_execute_data;
	}

	RETURN_STR_COPY(prev_execute_data->func->op_array.filename);
}
/* }}} */


/* {{{ proto bool ReflectionFiber::isStarted() */
ZEND_METHOD(ReflectionFiber, isStarted)
{
	zend_fiber_reflection *reflection;

	ZEND_PARSE_PARAMETERS_NONE();

	reflection = (zend_fiber_reflection *) Z_OBJ_P(getThis());

	RETURN_BOOL(reflection->fiber->status != ZEND_FIBER_STATUS_INIT);
}
/* }}} */


/* {{{ proto bool ReflectionFiber::isSuspended() */
ZEND_METHOD(ReflectionFiber, isSuspended)
{
	zend_fiber_reflection *reflection;

	ZEND_PARSE_PARAMETERS_NONE();

	reflection = (zend_fiber_reflection *) Z_OBJ_P(getThis());

	RETURN_BOOL(reflection->fiber->status == ZEND_FIBER_STATUS_SUSPENDED);
}
/* }}} */


/* {{{ proto bool ReflectionFiber::isRunning() */
ZEND_METHOD(ReflectionFiber, isRunning)
{
	zend_fiber_reflection *reflection;

	ZEND_PARSE_PARAMETERS_NONE();

	reflection = (zend_fiber_reflection *) Z_OBJ_P(getThis());

	RETURN_BOOL(reflection->fiber->status == ZEND_FIBER_STATUS_RUNNING);
}
/* }}} */


/* {{{ proto bool ReflectionFiber::isTerminated() */
ZEND_METHOD(ReflectionFiber, isTerminated)
{
	zend_fiber_reflection *reflection;

	ZEND_PARSE_PARAMETERS_NONE();

	reflection = (zend_fiber_reflection *) Z_OBJ_P(getThis());

	RETURN_BOOL(reflection->fiber->status & ZEND_FIBER_STATUS_FINISHED);
}
/* }}} */



ZEND_BEGIN_ARG_INFO_EX(arginfo_fiber_construct, 0, 0, 1)
	ZEND_ARG_CALLABLE_INFO(0, callable, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_fiber_start, 0, 0, 0)
	ZEND_ARG_VARIADIC_INFO(0, arguments)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_fiber_resume, 0, 0, 0)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_fiber_throw, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, exception, Throwable, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_fiber_getReturn, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fiber_status, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_fiber_this, 0, 0, Fiber, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_fiber_suspend, 0, 0, 0)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

static const zend_function_entry fiber_methods[] = {
	ZEND_ME(Fiber, __construct, arginfo_fiber_construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	ZEND_ME(Fiber, start, arginfo_fiber_start, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, resume, arginfo_fiber_resume, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, throw, arginfo_fiber_throw, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, isStarted, arginfo_fiber_status, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, isSuspended, arginfo_fiber_status, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, isRunning, arginfo_fiber_status, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, isTerminated, arginfo_fiber_status, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, getReturn, arginfo_fiber_getReturn, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, this, arginfo_fiber_this, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME(Fiber, suspend, arginfo_fiber_suspend, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_FE_END
};

ZEND_BEGIN_ARG_INFO(arginfo_fiber_error_create, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry fiber_error_methods[] = {
	ZEND_ME(FiberError, __construct, arginfo_fiber_error_create, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	ZEND_FE_END
};

ZEND_BEGIN_ARG_INFO(arginfo_reflection_fiber_construct, 0)
	ZEND_ARG_OBJ_INFO(0, fiber, Fiber, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_reflection_fiber_getFiber, 0, 0, Fiber, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_reflection_fiber_getExecutingLine, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_reflection_fiber_getExecutingFile, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_reflection_fiber_getTrace, 0, 0, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, options, IS_LONG, 0, "DEBUG_BACKTRACE_PROVIDE_OBJECT")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_reflection_fiber_status, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry reflection_fiber_methods[] = {
	ZEND_ME(ReflectionFiber, __construct, arginfo_reflection_fiber_construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	ZEND_ME(ReflectionFiber, getFiber, arginfo_reflection_fiber_getFiber, ZEND_ACC_PUBLIC)
	ZEND_ME(ReflectionFiber, getTrace, arginfo_reflection_fiber_getTrace, ZEND_ACC_PUBLIC)
	ZEND_ME(ReflectionFiber, getExecutingLine, arginfo_reflection_fiber_getExecutingLine, ZEND_ACC_PUBLIC)
	ZEND_ME(ReflectionFiber, getExecutingFile, arginfo_reflection_fiber_getExecutingFile, ZEND_ACC_PUBLIC)
	ZEND_ME(ReflectionFiber, isStarted, arginfo_reflection_fiber_status, ZEND_ACC_PUBLIC)
	ZEND_ME(ReflectionFiber, isSuspended, arginfo_reflection_fiber_status, ZEND_ACC_PUBLIC)
	ZEND_ME(ReflectionFiber, isRunning, arginfo_reflection_fiber_status, ZEND_ACC_PUBLIC)
	ZEND_ME(ReflectionFiber, isTerminated, arginfo_reflection_fiber_status, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};


void zend_fiber_ce_register(void)
{
	zend_class_entry ce;

	FIBER_G(catch_handler) = zend_get_user_opcode_handler(ZEND_CATCH);
	zend_set_user_opcode_handler(ZEND_CATCH, zend_fiber_catch_handler);

	INIT_CLASS_ENTRY(ce, "Fiber", fiber_methods);
	zend_ce_fiber = zend_register_internal_class(&ce);
	zend_ce_fiber->ce_flags |= ZEND_ACC_FINAL;
	zend_ce_fiber->create_object = zend_fiber_object_create;
	zend_ce_fiber->serialize = zend_class_serialize_deny;
	zend_ce_fiber->unserialize = zend_class_unserialize_deny;

	zend_fiber_handlers = std_object_handlers;
	zend_fiber_handlers.free_obj = zend_fiber_object_destroy;
	zend_fiber_handlers.clone_obj = NULL;

	INIT_CLASS_ENTRY(ce, "FiberError", fiber_error_methods);
	zend_ce_fiber_error = zend_register_internal_class_ex(&ce, zend_ce_error);
	zend_ce_fiber_error->ce_flags |= ZEND_ACC_FINAL;
	zend_ce_fiber_error->create_object = zend_ce_error->create_object;

	INIT_CLASS_ENTRY(ce, "FiberExit", fiber_error_methods);
	zend_ce_fiber_exit = zend_register_internal_class_ex(&ce, zend_ce_exception);
	zend_ce_fiber_exit->ce_flags |= ZEND_ACC_FINAL;
	zend_ce_fiber_exit->create_object = zend_ce_exception->create_object;

	INIT_CLASS_ENTRY(ce, "ReflectionFiber", reflection_fiber_methods);
	zend_ce_reflection_fiber = zend_register_internal_class(&ce);
	zend_ce_fiber_exit->ce_flags |= ZEND_ACC_FINAL;
	zend_ce_reflection_fiber->create_object = zend_reflection_fiber_object_create;
	zend_ce_reflection_fiber->serialize = zend_class_serialize_deny;
	zend_ce_reflection_fiber->unserialize = zend_class_unserialize_deny;

	zend_reflection_fiber_handlers = std_object_handlers;
	zend_reflection_fiber_handlers.free_obj = zend_reflection_fiber_object_destroy;
	zend_reflection_fiber_handlers.clone_obj = NULL;

	zend_llist_init(&zend_fiber_observers_list, sizeof(zend_observer_fiber_switch_handler), NULL, 1);
}

void zend_fiber_ce_unregister(void)
{
	zend_set_user_opcode_handler(ZEND_CATCH, FIBER_G(catch_handler));

	zend_llist_destroy(&zend_fiber_observers_list);
}

void zend_fiber_startup(void)
{
	FIBER_G(current_fiber) = NULL;
	FIBER_G(shutdown) = 0;
	FIBER_G(id) = 0;
}

void zend_fiber_shutdown(void)
{
	if (!FIBER_G(shutdown)) {
		zend_fiber_clean_shutdown();
	}

	FIBER_G(shutdown) = 1;

	zend_fiber_shutdown_cleanup();
}
