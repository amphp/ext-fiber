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

#include "zend.h"
#include "zend_API.h"
#include "zend_vm.h"
#include "zend_portability.h"
#include "zend_interfaces.h"
#include "zend_exceptions.h"
#include "zend_observer.h"
#include "zend_builtin_functions.h"

#include "php_fiber.h"
#include "fiber.h"
#include "fiber_arginfo.h"

PHP_FIBER_API zend_class_entry *zend_ce_fiber;

static zend_class_entry *zend_ce_reflection_fiber;

static zend_class_entry *zend_ce_fiber_error;
static zend_class_entry *zend_ce_fiber_exit;

static zend_object_handlers zend_fiber_handlers;
static zend_object_handlers zend_reflection_fiber_handlers;

static zend_function zend_fiber_function = { ZEND_INTERNAL_FUNCTION };

static zend_llist zend_fiber_observers_list;

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

#if __has_attribute(force_align_arg_pointer)
# define ZEND_STACK_ALIGNED __attribute__((force_align_arg_pointer))
#else
# define ZEND_STACK_ALIGNED
#endif



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

static void zend_fiber_suspend(zend_fiber *fiber)
{
	zend_vm_stack stack;
	size_t stack_page_size;
	zend_execute_data *execute_data;
	int error_reporting;
	uint32_t jit_trace_num;

	ZEND_FIBER_BACKUP_EG(stack, stack_page_size, execute_data, error_reporting, jit_trace_num);

	zend_fiber_suspend_context(&fiber->context);

	ZEND_FIBER_RESTORE_EG(stack, stack_page_size, execute_data, error_reporting, jit_trace_num);
}

static void zend_fiber_switch_to(zend_fiber *fiber)
{
	zend_fiber *previous;
	zend_vm_stack stack;
	size_t stack_page_size;
	zend_execute_data *execute_data;
	int error_reporting;
	uint32_t jit_trace_num;

	previous = FIBER_G(current_fiber);

	zend_observer_fiber_switch_notify(previous, fiber);

	ZEND_FIBER_BACKUP_EG(stack, stack_page_size, execute_data, error_reporting, jit_trace_num);

	FIBER_G(current_fiber) = fiber;

	zend_fiber_switch_context(&fiber->context);

	FIBER_G(current_fiber) = previous;

	ZEND_FIBER_RESTORE_EG(stack, stack_page_size, execute_data, error_reporting, jit_trace_num);

	zend_observer_fiber_switch_notify(fiber, previous);

	if (UNEXPECTED(FIBER_G(error)) && fiber->status != ZEND_FIBER_STATUS_SHUTDOWN) {
		if (previous) {
			zend_fiber_suspend(previous); // Still in fiber, suspend again until in {main}.
			abort(); // This fiber should never be resumed.
		}

		zend_fiber_error *error = FIBER_G(error);
		zend_error_at_noreturn(error->type, error->filename, error->lineno, "%s", ZSTR_VAL(error->message));
	}
}

static zend_always_inline zend_vm_stack zend_fiber_vm_stack_alloc(size_t size)
{
	zend_vm_stack page = emalloc(size);

	page->top = ZEND_VM_STACK_ELEMENTS(page);
	page->end = (zval *) ((uintptr_t) page + size);
	page->prev = NULL;

	return page;
}

static void ZEND_STACK_ALIGNED zend_fiber_execute(zend_fiber_context *context)
{
	zend_fiber *fiber = FIBER_G(current_fiber);
	ZEND_ASSERT(fiber);

	zend_long error_reporting = INI_INT("error_reporting");
	if (!error_reporting && !INI_STR("error_reporting")) {
		error_reporting = E_ALL;
	}

	zend_vm_stack stack = zend_fiber_vm_stack_alloc(ZEND_FIBER_VM_STACK_SIZE);
	EG(vm_stack) = stack;
	EG(vm_stack_top) = stack->top + ZEND_CALL_FRAME_SLOT;
	EG(vm_stack_end) = stack->end;
	EG(vm_stack_page_size) = ZEND_FIBER_VM_STACK_SIZE;

	fiber->execute_data = (zend_execute_data *) stack->top;
	fiber->stack_bottom = fiber->execute_data;

	memset(fiber->execute_data, 0, sizeof(zend_execute_data));

	fiber->execute_data->func = &zend_fiber_function;
	fiber->stack_bottom->prev_execute_data = EG(current_execute_data);

	EG(current_execute_data) = fiber->execute_data;
	EG(jit_trace_num) = 0;
	EG(error_reporting) = error_reporting;

	fiber->fci.retval = &fiber->value;

	fiber->status = ZEND_FIBER_STATUS_RUNNING;

	zend_call_function(&fiber->fci, &fiber->fci_cache);

	zval_ptr_dtor(&fiber->fci.function_name);

	if (EG(exception)) {
		if (fiber->status == ZEND_FIBER_STATUS_SHUTDOWN) {
			if (EXPECTED(zend_is_fiber_exit(EG(exception)) || zend_is_unwind_exit(EG(exception)))) {
				zend_clear_exception();
			}
		} else {
			fiber->status = ZEND_FIBER_STATUS_THREW;
		}
	} else {
		fiber->status = ZEND_FIBER_STATUS_RETURNED;
	}

	zend_vm_stack_destroy();
	fiber->execute_data = NULL;
	fiber->stack_bottom = NULL;
}

static zend_object *zend_fiber_object_create(zend_class_entry *ce)
{
	zend_fiber *fiber;

	fiber = emalloc(sizeof(zend_fiber));
	memset(fiber, 0, sizeof(zend_fiber));

	zend_object_std_init(&fiber->std, ce);
	fiber->std.handlers = &zend_fiber_handlers;

	return &fiber->std;
}

static void zend_fiber_object_destroy(zend_object *object)
{
	zend_fiber *fiber = (zend_fiber *) object;

	if (fiber->status != ZEND_FIBER_STATUS_SUSPENDED) {
		return;
	}

	zend_object *exception = EG(exception);
	EG(exception) = NULL;

	fiber->status = ZEND_FIBER_STATUS_SHUTDOWN;

	zend_fiber_switch_to(fiber);

	if (EG(exception)) {
		zend_exception_set_previous(EG(exception), exception);

		if (EG(flags) & EG_FLAGS_IN_SHUTDOWN) {
			zend_exception_error(EG(exception), E_ERROR);
		}
	} else {
		EG(exception) = exception;
	}
}

static void zend_fiber_object_free(zend_object *object)
{
	zend_fiber *fiber = (zend_fiber *) object;

	if (fiber->status == ZEND_FIBER_STATUS_INIT) {
		// Fiber was never started, so we need to release the reference to the callback.
		zval_ptr_dtor(&fiber->fci.function_name);
	}

	zval_ptr_dtor(&fiber->value);

	zend_fiber_destroy_context(&fiber->context);

	zend_object_std_dtor(&fiber->std);
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

void zend_fiber_error_observer(int type, const char *filename, uint32_t line, zend_string *message)
{
	if (!(type & E_FATAL_ERRORS)) {
		return; // Non-fatal error, nothing to do.
	}

	zend_fiber *fiber = FIBER_G(current_fiber);

	if (fiber) {
		// In a fiber, we need to switch back to main.
		zend_fiber_error error;

		error.type = type;
		error.filename = filename;
		error.lineno = line;
		error.message = message;

		FIBER_G(error) = &error;

		zend_fiber_suspend(fiber);

		abort(); // This fiber should never be resumed.
	}
}

static zend_object *zend_reflection_fiber_object_create(zend_class_entry *ce)
{
	zend_fiber_reflection *reflection;

	reflection = emalloc(sizeof(zend_fiber_reflection));
	memset(reflection, 0, sizeof(zend_fiber_reflection));

	zend_object_std_init(&reflection->std, ce);
	reflection->std.handlers = &zend_reflection_fiber_handlers;

	return &reflection->std;
}

static void zend_reflection_fiber_object_free(zend_object *object)
{
	zend_fiber_reflection *reflection = (zend_fiber_reflection *) object;

	if (reflection->fiber) {
		GC_DELREF(&reflection->fiber->std);
	}

	zend_object_std_dtor(&reflection->std);
}

ZEND_METHOD(Fiber, __construct)
{
	zend_fiber *fiber = (zend_fiber *) Z_OBJ_P(getThis());

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_FUNC(fiber->fci, fiber->fci_cache)
	ZEND_PARSE_PARAMETERS_END();

	// Keep a reference to closures or callable objects until the fiber is started.
	Z_TRY_ADDREF(fiber->fci.function_name);
}

ZEND_METHOD(Fiber, start)
{
	zend_fiber *fiber = (zend_fiber *) Z_OBJ_P(getThis());
	zval *params;
	uint32_t param_count;
	zend_array *named_params;

	ZEND_PARSE_PARAMETERS_START(0, -1)
		Z_PARAM_VARIADIC_WITH_NAMED(params, param_count, named_params);
	ZEND_PARSE_PARAMETERS_END();

	if (fiber->status != ZEND_FIBER_STATUS_INIT) {
		zend_throw_error(zend_ce_fiber_error, "Cannot start a fiber that has already been started");
		RETURN_THROWS();
	}

	fiber->fci.params = params;
	fiber->fci.param_count = param_count;
	fiber->fci.named_params = named_params;

	if (!zend_fiber_init_context(&fiber->context, zend_fiber_execute, FIBER_G(stack_size))) {
		zend_throw_exception(NULL, "Failed to create native fiber context", 0);
		RETURN_THROWS();
	}

	zend_fiber_switch_to(fiber);

	if (fiber->status & ZEND_FIBER_STATUS_FINISHED) {
		RETURN_NULL();
	}

	RETVAL_COPY_VALUE(&fiber->value);
	ZVAL_UNDEF(&fiber->value);
}

ZEND_METHOD(Fiber, suspend)
{
	zend_fiber *fiber = FIBER_G(current_fiber);
	zval *exception, *value = NULL;

	ZEND_PARSE_PARAMETERS_START(0, 1)
		Z_PARAM_OPTIONAL
		Z_PARAM_ZVAL(value);
	ZEND_PARSE_PARAMETERS_END();

	if (UNEXPECTED(!fiber)) {
		zend_throw_error(zend_ce_fiber_error, "Cannot suspend outside of a fiber");
		RETURN_THROWS();
	}

	if (UNEXPECTED(fiber->status == ZEND_FIBER_STATUS_SHUTDOWN)) {
		zend_throw_error(zend_ce_fiber_error, "Cannot suspend in a force-closed fiber");
		RETURN_THROWS();
	}

	ZEND_ASSERT(fiber->status == ZEND_FIBER_STATUS_RUNNING);

	if (value) {
		ZVAL_COPY(&fiber->value, value);
	} else {
		ZVAL_NULL(&fiber->value);
	}

	fiber->execute_data = execute_data;
	fiber->status = ZEND_FIBER_STATUS_SUSPENDED;
	fiber->stack_bottom->prev_execute_data = NULL;

	zend_fiber_suspend(fiber);

	if (fiber->status == ZEND_FIBER_STATUS_SHUTDOWN) {
		// This occurs on exit if the fiber never resumed, it has been GC'ed, so do not add a ref.
		if (FIBER_G(error)) {
			// Throw UnwindExit so finally blocks are not executed on fatal error.
			zend_throw_unwind_exit();
		} else {
			// Otherwise throw FiberExit to execute finally blocks.
			zend_throw_error(zend_ce_fiber_exit, "Fiber destroyed");
		}
		RETURN_THROWS();
	}

	fiber->status = ZEND_FIBER_STATUS_RUNNING;

	if (fiber->exception) {
		exception = fiber->exception;
		fiber->exception = NULL;

		zend_throw_exception_object(exception);
		RETURN_THROWS();
	}

	RETVAL_COPY_VALUE(&fiber->value);
	ZVAL_UNDEF(&fiber->value);
}

ZEND_METHOD(Fiber, resume)
{
	zend_fiber *fiber;
	zval *value = NULL;

	ZEND_PARSE_PARAMETERS_START(0, 1)
		Z_PARAM_OPTIONAL
		Z_PARAM_ZVAL(value);
	ZEND_PARSE_PARAMETERS_END();

	fiber = (zend_fiber *) Z_OBJ_P(getThis());

	if (UNEXPECTED(fiber->status != ZEND_FIBER_STATUS_SUSPENDED)) {
		zend_throw_error(zend_ce_fiber_error, "Cannot resume a fiber that is not suspended");
		RETURN_THROWS();
	}

	if (value) {
		ZVAL_COPY(&fiber->value, value);
	} else {
		ZVAL_NULL(&fiber->value);
	}

	fiber->status = ZEND_FIBER_STATUS_RUNNING;
	fiber->stack_bottom->prev_execute_data = execute_data;

	zend_fiber_switch_to(fiber);

	if (fiber->status & ZEND_FIBER_STATUS_FINISHED) {
		RETURN_NULL();
	}

	RETVAL_COPY_VALUE(&fiber->value);
	ZVAL_UNDEF(&fiber->value);
}

ZEND_METHOD(Fiber, throw)
{
	zend_fiber *fiber;
	zval *exception;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(exception, zend_ce_throwable)
	ZEND_PARSE_PARAMETERS_END();

	fiber = (zend_fiber *) Z_OBJ_P(getThis());

	if (UNEXPECTED(fiber->status != ZEND_FIBER_STATUS_SUSPENDED)) {
		zend_throw_error(zend_ce_fiber_error, "Cannot resume a fiber that is not suspended");
		RETURN_THROWS();
	}

	Z_ADDREF_P(exception);
	fiber->exception = exception;

	fiber->status = ZEND_FIBER_STATUS_RUNNING;
	fiber->stack_bottom->prev_execute_data = execute_data;

	zend_fiber_switch_to(fiber);

	if (fiber->status & ZEND_FIBER_STATUS_FINISHED) {
		RETURN_NULL();
	}

	RETVAL_COPY_VALUE(&fiber->value);
	ZVAL_UNDEF(&fiber->value);
}

ZEND_METHOD(Fiber, isStarted)
{
	zend_fiber *fiber;

	ZEND_PARSE_PARAMETERS_NONE();

	fiber = (zend_fiber *) Z_OBJ_P(getThis());

	RETURN_BOOL(fiber->status != ZEND_FIBER_STATUS_INIT);
}

ZEND_METHOD(Fiber, isSuspended)
{
	zend_fiber *fiber;

	ZEND_PARSE_PARAMETERS_NONE();

	fiber = (zend_fiber *) Z_OBJ_P(getThis());

	RETURN_BOOL(fiber->status == ZEND_FIBER_STATUS_SUSPENDED);
}

ZEND_METHOD(Fiber, isRunning)
{
	zend_fiber *fiber;

	ZEND_PARSE_PARAMETERS_NONE();

	fiber = (zend_fiber *) Z_OBJ_P(getThis());

	RETURN_BOOL(fiber->status == ZEND_FIBER_STATUS_RUNNING);
}

ZEND_METHOD(Fiber, isTerminated)
{
	zend_fiber *fiber;

	ZEND_PARSE_PARAMETERS_NONE();

	fiber = (zend_fiber *) Z_OBJ_P(getThis());

	RETURN_BOOL(fiber->status & ZEND_FIBER_STATUS_FINISHED);
}

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
		RETURN_THROWS();
	}

	RETURN_COPY(&fiber->value);
}

ZEND_METHOD(Fiber, this)
{
	zend_fiber *fiber;

	ZEND_PARSE_PARAMETERS_NONE();

	fiber = FIBER_G(current_fiber);

	if (!fiber) {
		RETURN_NULL();
	}

	RETURN_OBJ_COPY(&fiber->std);
}

ZEND_METHOD(FiberError, __construct)
{
	zend_object *object = Z_OBJ_P(getThis());

	zend_throw_error(
		NULL,
		"The \"%s\" class is reserved for internal use and cannot be manually instantiated",
		object->ce->name->val
	);
}

ZEND_METHOD(FiberExit, __construct)
{
	zend_object *object = Z_OBJ_P(getThis());

	zend_throw_error(
		NULL,
		"The \"%s\" class is reserved for internal use and cannot be manually instantiated",
		object->ce->name->val
	);
}

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

ZEND_METHOD(ReflectionFiber, getFiber)
{
	zend_fiber_reflection *reflection;

	ZEND_PARSE_PARAMETERS_NONE();

	reflection = (zend_fiber_reflection *) Z_OBJ_P(getThis());

	RETURN_OBJ_COPY(&reflection->fiber->std);
}

#define REFLECTION_CHECK_VALID_FIBER(fiber) do { \
		if (fiber == NULL || fiber->status == ZEND_FIBER_STATUS_INIT || fiber->status & ZEND_FIBER_STATUS_FINISHED) { \
			zend_throw_error(NULL, "Cannot fetch information from a fiber that has not been started or is terminated"); \
			return; \
		} \
	} while (0)

ZEND_METHOD(ReflectionFiber, getTrace)
{
	zend_fiber_reflection *reflection;
	zend_long options = DEBUG_BACKTRACE_PROVIDE_OBJECT;
	zend_execute_data *prev_execute_data;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 0, 1)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(options);
	ZEND_PARSE_PARAMETERS_END();

	reflection = (zend_fiber_reflection *) Z_OBJ_P(getThis());

	REFLECTION_CHECK_VALID_FIBER(reflection->fiber);

	prev_execute_data = reflection->fiber->stack_bottom->prev_execute_data;
	reflection->fiber->stack_bottom->prev_execute_data = NULL;

	if (FIBER_G(current_fiber) != reflection->fiber) {
		// No need to replace current execute data if within the current fiber.
		EG(current_execute_data) = reflection->fiber->execute_data;
	}

	zend_fetch_debug_backtrace(return_value, 0, options, 0);

	EG(current_execute_data) = execute_data; // Restore original execute data.
	reflection->fiber->stack_bottom->prev_execute_data = prev_execute_data;
}

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

ZEND_METHOD(ReflectionFiber, getCallable)
{
	zend_fiber_reflection *reflection;
	zend_fiber *fiber;

	ZEND_PARSE_PARAMETERS_NONE();

	reflection = (zend_fiber_reflection *) Z_OBJ_P(getThis());
	fiber = reflection->fiber;

	if (fiber == NULL || fiber->status & ZEND_FIBER_STATUS_FINISHED) {
		zend_throw_error(NULL, "Cannot fetch the callable from a fiber that has terminated"); \
		RETURN_THROWS();
	}

	RETURN_COPY(&fiber->fci.function_name);
}

void zend_register_fiber_ce(void)
{
	FIBER_G(catch_handler) = zend_get_user_opcode_handler(ZEND_CATCH);
	zend_set_user_opcode_handler(ZEND_CATCH, zend_fiber_catch_handler);

	zend_ce_fiber = register_class_Fiber();
	zend_ce_fiber->create_object = zend_fiber_object_create;
	zend_ce_fiber->serialize = zend_class_serialize_deny;
	zend_ce_fiber->unserialize = zend_class_unserialize_deny;

	zend_fiber_handlers = std_object_handlers;
	zend_fiber_handlers.dtor_obj = zend_fiber_object_destroy;
	zend_fiber_handlers.free_obj = zend_fiber_object_free;
	zend_fiber_handlers.clone_obj = NULL;

	zend_ce_fiber_error = register_class_FiberError(zend_ce_error);
	zend_ce_fiber_error->create_object = zend_ce_error->create_object;

	zend_ce_fiber_exit = register_class_FiberExit(zend_ce_exception);
	zend_ce_fiber_exit->create_object = zend_ce_exception->create_object;

	zend_ce_reflection_fiber = register_class_ReflectionFiber();
	zend_ce_reflection_fiber->create_object = zend_reflection_fiber_object_create;
	zend_ce_reflection_fiber->serialize = zend_class_serialize_deny;
	zend_ce_reflection_fiber->unserialize = zend_class_unserialize_deny;

	zend_reflection_fiber_handlers = std_object_handlers;
	zend_reflection_fiber_handlers.free_obj = zend_reflection_fiber_object_free;
	zend_reflection_fiber_handlers.clone_obj = NULL;

	zend_llist_init(&zend_fiber_observers_list, sizeof(zend_observer_fiber_switch_handler), NULL, 1);
}

void zend_fiber_shutdown(void)
{
	zend_set_user_opcode_handler(ZEND_CATCH, FIBER_G(catch_handler));

	zend_llist_destroy(&zend_fiber_observers_list);
}

void zend_fiber_init(void)
{
	FIBER_G(current_fiber) = NULL;
}
