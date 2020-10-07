/*
  +--------------------------------------------------------------------+
  | ext-fiber                                                          |
  +--------------------------------------------------------------------+
  | Redistribution and use in source and binary forms, with or without |
  | modification, are permitted provided that the conditions mentioned |
  | in the accompanying LICENSE file are met.                          |
  +--------------------------------------------------------------------+
  | Authors: Martin Schröder <m.schroeder2007@gmail.com>               |
  |          Aaron Piotrowski <aaron@trowski.com>                      |
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

#include "php_fiber.h"
#include "fiber.h"

#ifndef ZEND_PARSE_PARAMETERS_NONE
#define ZEND_PARSE_PARAMETERS_NONE() zend_parse_parameters_none()
#endif

ZEND_API zend_class_entry *zend_ce_awaitable;
ZEND_API zend_class_entry *zend_ce_fiber_scheduler;

static zend_class_entry *zend_ce_fiber;
static zend_class_entry *zend_ce_fiber_error;
static zend_object_handlers zend_fiber_handlers;

static zend_object *zend_fiber_object_create(zend_class_entry *ce);
static zend_object *zend_fiber_object_initialize(zend_class_entry *ce, zend_bool is_scheduler);
static void zend_fiber_object_destroy(zend_object *object);

static zend_op_array fiber_run_func;
static zend_try_catch_element fiber_terminate_try_catch_array = { 0, 1, 0, 0 };
static zend_op fiber_run_op[2];

static HashTable fibers;
static HashTable schedulers;
static zend_string *scheduler_run_name;
static zend_string *fiber_continue_name;

#define ZEND_FIBER_BACKUP_EG(stack, stack_page_size, exec) do { \
	stack = EG(vm_stack); \
	stack->top = EG(vm_stack_top); \
	stack->end = EG(vm_stack_end); \
	stack_page_size = EG(vm_stack_page_size); \
	exec = EG(current_execute_data); \
} while (0)

#define ZEND_FIBER_RESTORE_EG(stack, stack_page_size, exec) do { \
	EG(vm_stack) = stack; \
	EG(vm_stack_top) = stack->top; \
	EG(vm_stack_end) = stack->end; \
	EG(vm_stack_page_size) = stack_page_size; \
	EG(current_execute_data) = exec; \
} while (0)


static zend_always_inline zend_bool zend_fiber_is_scheduler(zend_fiber *fiber)
{
	return fiber->continuation == NULL;
}


static zend_fiber *zend_fiber_create_root()
{
	zend_fiber *root_fiber;

	root_fiber = (zend_fiber *) zend_fiber_object_initialize(zend_ce_fiber, 0);
	root_fiber->stack_size = 0;
	root_fiber->stack = NULL;
	root_fiber->context = zend_fiber_create_root_context();

	root_fiber->status = ZEND_FIBER_STATUS_RUNNING;

	FIBER_G(root_fiber) = root_fiber;

	return root_fiber;
}


static zend_bool zend_fiber_switch_to(zend_fiber *fiber)
{
	zend_fiber *previous;
	zend_bool result;
	zend_execute_data *exec;
	zend_vm_stack stack;
	size_t stack_page_size;

	previous = FIBER_G(current_fiber);

	if (previous == NULL) {
		previous = zend_fiber_create_root();
	}

	FIBER_G(current_fiber) = fiber;

	ZEND_FIBER_BACKUP_EG(stack, stack_page_size, exec);

	result = zend_fiber_switch_context(previous->context, fiber->context);

	FIBER_G(current_fiber) = previous;

	ZEND_FIBER_RESTORE_EG(stack, stack_page_size, exec);

	return result;
}


static zend_bool zend_fiber_suspend(zend_fiber *fiber)
{
	size_t stack_page_size;
	zend_bool result;

	if (fiber->stack == NULL) {
		return 1;
	}

	ZEND_FIBER_BACKUP_EG(fiber->stack, stack_page_size, fiber->exec);

	result = zend_fiber_suspend_context(fiber->context);

	ZEND_FIBER_RESTORE_EG(fiber->stack, stack_page_size, fiber->exec);

	return result;
}


static void zend_fiber_run()
{
	zend_fiber *fiber;

	fiber = FIBER_G(current_fiber);
	ZEND_ASSERT(fiber != NULL);

	EG(vm_stack) = fiber->stack;
	EG(vm_stack_top) = fiber->stack->top;
	EG(vm_stack_end) = fiber->stack->end;
	EG(vm_stack_page_size) = ZEND_FIBER_VM_STACK_SIZE;

	fiber->exec = (zend_execute_data *) EG(vm_stack_top);
	EG(vm_stack_top) = (zval *) fiber->exec + ZEND_CALL_FRAME_SLOT;
	zend_vm_init_call_frame(fiber->exec, ZEND_CALL_TOP_FUNCTION, (zend_function *) &fiber_run_func, 0, NULL);
	fiber->exec->opline = fiber_run_op;
	fiber->exec->call = NULL;
	fiber->exec->return_value = NULL;
	fiber->exec->prev_execute_data = NULL;

	EG(current_execute_data) = fiber->exec;

	execute_ex(fiber->exec);

	zend_vm_stack_destroy();
	fiber->stack = NULL;
	fiber->exec = NULL;

	if (fiber->continuation != NULL) {
		zval_ptr_dtor(&fiber->continuation->value);
		zval_ptr_dtor(&fiber->continuation->closure);
	}

	zend_hash_index_del(&fibers, fiber->std.handle);

	zend_fiber_suspend_context(fiber->context);

	abort();
}


static int fiber_run_opcode_handler(zend_execute_data *exec)
{
	zend_fiber *fiber;
	zval retval;

	fiber = FIBER_G(current_fiber);
	ZEND_ASSERT(fiber != NULL && fiber != FIBER_G(root_fiber));

	fiber->status = ZEND_FIBER_STATUS_RUNNING;
	fiber->fci.retval = &retval;

	zend_call_function(&fiber->fci, &fiber->fci_cache);

	zval_ptr_dtor(&fiber->fci.function_name);

	if (EG(exception)) {
		if (fiber->status == ZEND_FIBER_STATUS_DEAD) {
			zend_clear_exception();
		} else {
			fiber->status = ZEND_FIBER_STATUS_DEAD;
		}
	} else {
		fiber->status = ZEND_FIBER_STATUS_FINISHED;
	}

	return ZEND_USER_OPCODE_RETURN;
}


static zend_bool zend_fiber_resume(zend_fiber *fiber, zend_fiber *scheduler)
{
	zend_bool result;

	ZEND_ASSERT(zend_fiber_is_scheduler(scheduler));

	if (scheduler->link == fiber) {
		// Suspend from scheduler to fiber that resumed the scheduler.
		scheduler->status = ZEND_FIBER_STATUS_SUSPENDED;
		scheduler->link = NULL;
		result = zend_fiber_suspend(scheduler);
		scheduler->status = ZEND_FIBER_STATUS_RUNNING;
		return result;
	}

	// Another fiber started the scheduler, so switch to resuming fiber.
	return zend_fiber_switch_to(fiber);
}


static zend_object *zend_fiber_object_create(zend_class_entry *ce)
{
	/* This function is called if `new Fiber` is used. It will be immediately
	 * destroyed after an exception is thrown by zend_fiber_get_constructor. */

	zend_fiber *fiber;

	fiber = (zend_fiber *) zend_fiber_object_initialize(ce, 1);
	GC_ADDREF(&fiber->std);

	return &fiber->std;
}


static zend_object *zend_fiber_object_initialize(zend_class_entry *ce, zend_bool is_scheduler)
{
	zend_fiber *fiber;
	zend_function *func;
	zval context;

	fiber = emalloc(sizeof(zend_fiber));
	memset(fiber, 0, sizeof(zend_fiber));

	zend_object_std_init(&fiber->std, ce);
	fiber->std.handlers = &zend_fiber_handlers;

	ZVAL_OBJ(&context, &fiber->std);
	Z_ADDREF(context);

	if (!is_scheduler) {
		fiber->continuation = emalloc(sizeof(zend_fiber_continuation));
		memset(fiber->continuation, 0, sizeof(zend_fiber_continuation));

		func = zend_hash_find_ptr(&ce->function_table, fiber_continue_name);
		zend_create_fake_closure(&fiber->continuation->closure, func, func->op_array.scope, ce, &context);

		ZVAL_UNDEF(&fiber->continuation->value);
	}

	zval_ptr_dtor(&context);

	zend_hash_index_add_ptr(&fibers, fiber->std.handle, fiber);

	return &fiber->std;
}


static void zend_fiber_object_destroy(zend_object *object)
{
	zend_fiber *fiber;

	fiber = (zend_fiber *) object;

	if (fiber->status == ZEND_FIBER_STATUS_SUSPENDED) {
		fiber->status = ZEND_FIBER_STATUS_DEAD;

		zend_fiber_switch_to(fiber);
	}

	if (fiber->continuation != NULL) {
		efree(fiber->continuation);
	}

	zend_fiber_destroy(fiber->context);

	zend_object_std_dtor(&fiber->std);
}


static void zend_fiber_hash_index_destroy(zval *ptr)
{
	zend_fiber *fiber;

	fiber = (zend_fiber *) Z_PTR_P(ptr);
	GC_DELREF(&fiber->std);
}


static zend_fiber *zend_fiber_create_from_scheduler(zval *scheduler)
{
	zend_fiber *fiber;
	zend_function *func;
	zval closure;

	ZEND_ASSERT(instanceof_function(Z_OBJCE_P(scheduler), zend_ce_fiber_scheduler));

	fiber = (zend_fiber *) zend_fiber_object_initialize(zend_ce_fiber, 1);

	func = zend_hash_find_ptr(&(Z_OBJCE_P(scheduler)->function_table), scheduler_run_name);
	zend_create_fake_closure(&closure, func, func->op_array.scope, Z_OBJCE_P(scheduler), scheduler);

	if (zend_fcall_info_init(&closure, 0, &fiber->fci, &fiber->fci_cache, NULL, NULL) == FAILURE) {
		zval_ptr_dtor(&closure);
		zend_throw_error(zend_ce_fiber_error, "Failed initializing FiberScheduler::run() fcall info");
		return NULL;
	}

	// Keep a reference to closures or callable objects as long as the fiber lives.
	Z_TRY_ADDREF(fiber->fci.function_name);

	zval_ptr_dtor(&closure);

	fiber->context = zend_fiber_create_context();
	fiber->stack_size = FIBER_G(stack_size);

	if (fiber->context == NULL) {
		zend_throw_error(zend_ce_fiber_error, "Failed creating scheduler fiber context");
		return NULL;
	}

	if (!zend_fiber_create(fiber->context, zend_fiber_run, fiber->stack_size)) {
		zend_throw_error(zend_ce_fiber_error, "Failed creating scheduler fiber");
		return NULL;
	}

	fiber->stack = (zend_vm_stack) emalloc(ZEND_FIBER_VM_STACK_SIZE);
	fiber->stack->top = ZEND_VM_STACK_ELEMENTS(fiber->stack) + 1;
	fiber->stack->end = (zval *) ((char *) fiber->stack + ZEND_FIBER_VM_STACK_SIZE);
	fiber->stack->prev = NULL;

	fiber->status = ZEND_FIBER_STATUS_INIT;

	return fiber;
}


static zend_fiber *zend_fiber_get_scheduler(zval *scheduler)
{
	zend_fiber *fiber;
	zend_ulong handle = Z_OBJ_HANDLE_P(scheduler);

	ZEND_ASSERT(instanceof_function(Z_OBJCE_P(scheduler), zend_ce_fiber_scheduler));

	fiber = zend_hash_index_find_ptr(&schedulers, handle);

	if (fiber != NULL) {
		if (UNEXPECTED(fiber->status == ZEND_FIBER_STATUS_FINISHED || fiber->status == ZEND_FIBER_STATUS_DEAD)) {
			zend_throw_error(zend_ce_fiber_error, "FiberScheduler::run() returned unexpectedly");
			return NULL;
		}

		return fiber;
	}

	fiber = zend_fiber_create_from_scheduler(scheduler);

	if (fiber == NULL) {
		// FiberError thrown in zend_fiber_create_from_scheduler.
		return NULL;
	}

	zend_hash_index_add_ptr(&schedulers, handle, fiber);

	return fiber;
}


static void zend_fiber_cleanup()
{
	zend_fiber *fiber;
	uint32_t handle;

	ZEND_HASH_REVERSE_FOREACH_NUM_KEY_PTR(&fibers, handle, fiber) {
		if (fiber->status == ZEND_FIBER_STATUS_SUSPENDED && !zend_fiber_is_scheduler(fiber)) {
			fiber->status = ZEND_FIBER_STATUS_DEAD;
			zend_fiber_switch_to(fiber);
		}

		zend_hash_index_del(&fibers, handle);
	} ZEND_HASH_FOREACH_END();

	ZEND_HASH_REVERSE_FOREACH_NUM_KEY_PTR(&schedulers, handle, fiber) {
		zend_hash_index_del(&schedulers, handle);
		efree(fiber);
	} ZEND_HASH_FOREACH_END();
}


static void zend_fiber_observer_end(zend_execute_data *execute_data, zval *retval)
{
	zend_fiber *fiber;

	ZEND_HASH_REVERSE_FOREACH_PTR(&schedulers, fiber) {
		if (fiber->status == ZEND_FIBER_STATUS_SUSPENDED) {
			fiber->status = ZEND_FIBER_STATUS_RUNNING;
			zend_fiber_switch_to(fiber);
		}
	} ZEND_HASH_FOREACH_END();
}


zend_observer_fcall_handlers zend_fiber_observer_fcall_init(zend_execute_data *execute_data)
{
	if (!execute_data->func->common.function_name && !execute_data->prev_execute_data) {
		return (zend_observer_fcall_handlers){NULL, zend_fiber_observer_end};
	}
	
	return (zend_observer_fcall_handlers){NULL, NULL};
}


static void zend_fiber_scheduler_uncaught_exception_handler(zend_object *exception)
{
	zval param, retval, name, message;

	ZEND_ASSERT(instanceof_function(exception->ce, zend_ce_throwable));

	if (Z_TYPE(EG(user_exception_handler)) != IS_UNDEF) {
		GC_ADDREF(exception);
		ZVAL_OBJ(&param, exception);

		zend_clear_exception();

		call_user_function(EG(function_table), NULL, &EG(user_exception_handler), &retval, 1, &param);

		zval_ptr_dtor(&param);
		zval_ptr_dtor(&retval);
	}

	ZVAL_STR(&name, exception->ce->name);
	ZVAL_COPY(&message, zend_read_property(exception->ce, exception, "message", sizeof("message") - 1, 0, &retval));

	zend_error(E_ERROR, "Uncaught %s thrown from FiberScheduler::run(): %s", Z_STRVAL(name), Z_STRVAL(message));

	zval_ptr_dtor(&name);
	zval_ptr_dtor(&message);
	zval_ptr_dtor(&retval);
}


static ZEND_COLD zend_function *zend_fiber_get_constructor(zend_object *object)
{
	zend_throw_error(NULL, "The \"Fiber\" class is reserved for internal use and cannot be manually instantiated");

	return NULL;
}


/* {{{ proto void Fiber::run(callable $callback, mixed ...$args) */
ZEND_METHOD(Fiber, run)
{
	zend_fiber *fiber;
	zval *params;
	uint32_t param_count;
	
	fiber = FIBER_G(current_fiber);
	
	if (fiber == NULL || !zend_fiber_is_scheduler(fiber)) {
		zend_throw_error(NULL, "New fibers can only be created inside FiberScheduler::run()");
		return;
	}

	fiber = (zend_fiber *) zend_fiber_object_initialize(zend_ce_fiber, 0);

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, -1)
		Z_PARAM_FUNC_EX(fiber->fci, fiber->fci_cache, 1, 0)
		Z_PARAM_VARIADIC('+', params, param_count)
	ZEND_PARSE_PARAMETERS_END();

	// Keep a reference to closures or callable objects as long as the fiber lives.
	Z_TRY_ADDREF(fiber->fci.function_name);

	fiber->fci.params = params;
	fiber->fci.param_count = param_count;

	fiber->context = zend_fiber_create_context();
	fiber->stack_size = FIBER_G(stack_size);

	if (fiber->context == NULL) {
		zend_throw_error(zend_ce_fiber_error, "Failed to create native fiber context");
		return;
	}

	if (!zend_fiber_create(fiber->context, zend_fiber_run, fiber->stack_size)) {
		zend_throw_error(zend_ce_fiber_error, "Failed to create native fiber");
		return;
	}

	fiber->stack = (zend_vm_stack) emalloc(ZEND_FIBER_VM_STACK_SIZE);
	fiber->stack->top = ZEND_VM_STACK_ELEMENTS(fiber->stack) + 1;
	fiber->stack->end = (zval *) ((char *) fiber->stack + ZEND_FIBER_VM_STACK_SIZE);
	fiber->stack->prev = NULL;

	fiber->status = ZEND_FIBER_STATUS_RUNNING;

	if (!zend_fiber_switch_to(fiber)) {
		fiber->status = ZEND_FIBER_STATUS_INIT;
		zend_throw_error(zend_ce_fiber_error, "Failed switching to fiber");
		return;
	}
}
/* }}} */


/* {{{ proto void Fiber::continue(?Throwable $exception, mixed $value) */
ZEND_METHOD(Fiber, continue)
{
	zend_fiber *fiber, *scheduler;
	zval *value = NULL, *exception = NULL;
	
	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS_EX(exception, zend_ce_throwable, 1, 0)
		Z_PARAM_ZVAL(value);
	ZEND_PARSE_PARAMETERS_END();

	scheduler = FIBER_G(current_fiber);
	
	if (UNEXPECTED(scheduler == NULL || !zend_fiber_is_scheduler(scheduler))) {
		zend_throw_error(zend_ce_fiber_error, "Fibers can only be resumed within a scheduler");
		return;
	}

	fiber = (zend_fiber *) Z_OBJ_P(getThis());
	
	if (fiber->link != scheduler) {
		zend_throw_error(zend_ce_fiber_error, "Fiber resumed by a scheduler other than that provided to await");
		return;
	}
	
	fiber->link = NULL;
	
	if (UNEXPECTED(fiber->status != ZEND_FIBER_STATUS_SUSPENDED)) {
		zend_throw_error(zend_ce_fiber_error, "Cannot resume running fiber");
		return;
	}

	if (exception != NULL) {
		if (value != NULL && Z_TYPE_P(value) != IS_NULL) {
			zend_throw_error(zend_ce_fiber_error, "$value must be NULL when $exception is not NULL");
			return;
		}
		
		Z_ADDREF_P(exception);
		fiber->continuation->error = exception;
	} else {
		ZVAL_COPY(&fiber->continuation->value, value);
	}

	fiber->status = ZEND_FIBER_STATUS_RUNNING;

	if (!zend_fiber_resume(fiber, scheduler)) {
		fiber->status = ZEND_FIBER_STATUS_SUSPENDED;
		zend_throw_error(zend_ce_fiber_error, "Failed resuming fiber");
		return;
	}
}


/* {{{ proto mixed Fiber::await(Awaitable $awaitable, FiberScheduler $scheduler) */
ZEND_METHOD(Fiber, await)
{
	zend_fiber *fiber, *scheduler;
	zend_execute_data *exec;
	zval *awaitable, *fiber_scheduler, *error, method_name, retval;

	fiber = FIBER_G(current_fiber);

	if (fiber == NULL) {
		fiber = zend_fiber_create_root();
		
		if (fiber == NULL) {
			zend_throw_error(zend_ce_fiber_error, "Could not create root fiber context");
			return;
		}
		
		FIBER_G(current_fiber) = fiber;
	} else {
		if (UNEXPECTED(zend_fiber_is_scheduler(fiber))) {
			zend_throw_error(zend_ce_fiber_error, "Cannot await in a scheduler");
			return;
		}

		if (UNEXPECTED(fiber->status != ZEND_FIBER_STATUS_RUNNING)) {
			zend_throw_error(zend_ce_fiber_error, "Cannot await in a fiber that is not running");
			return;
		}
	}

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
		Z_PARAM_OBJECT_OF_CLASS(awaitable, zend_ce_awaitable)
		Z_PARAM_OBJECT_OF_CLASS(fiber_scheduler, zend_ce_fiber_scheduler)
	ZEND_PARSE_PARAMETERS_END();

	scheduler = zend_fiber_get_scheduler(fiber_scheduler);

	if (scheduler == NULL) {
		// FiberError thrown in zend_fiber_get_scheduler.
		return;
	}

   	fiber->status = ZEND_FIBER_STATUS_SUSPENDED;

	ZVAL_STRING(&method_name, "onResolve");
	call_user_function(NULL, awaitable, &method_name, &retval, 1, &fiber->continuation->closure);
	zval_ptr_dtor(&retval);
	zval_ptr_dtor(&method_name);
	
	if (UNEXPECTED(EG(exception))) {
		// Exception thrown from Awaitable::onResolve().
		fiber->status = ZEND_FIBER_STATUS_RUNNING;
		return;
	}

	fiber->link = scheduler;

	if (scheduler->status == ZEND_FIBER_STATUS_RUNNING) {
		if (!zend_fiber_suspend(fiber)) {
			fiber->status = ZEND_FIBER_STATUS_RUNNING;
			zend_throw_error(zend_ce_fiber_error, "Failed suspending fiber");
			return;
		}
	} else {
		scheduler->status = ZEND_FIBER_STATUS_RUNNING;
		scheduler->link = fiber;

		if (!zend_fiber_switch_to(scheduler)) {
			fiber->status = ZEND_FIBER_STATUS_RUNNING;
			zend_throw_error(zend_ce_fiber_error, "Failed switching to scheduler");
			return;
		}
	}

	if (fiber->status == ZEND_FIBER_STATUS_DEAD) {
		zend_throw_error(zend_ce_fiber_error, "Fiber has been destroyed");
		return;
	}

	fiber->status = ZEND_FIBER_STATUS_RUNNING;
	
	if (UNEXPECTED(EG(exception))) {
		// Exception thrown from scheduler, invoke exception handler and bailout.
		if (zend_is_unwind_exit(EG(exception))) {
			return; // Exception is UnwindExit, so ignore as we are exiting anyway.
		}

		fiber->status = ZEND_FIBER_STATUS_DEAD;
		zend_fiber_scheduler_uncaught_exception_handler(EG(exception));
		return;
	}

	if (UNEXPECTED(scheduler->status == ZEND_FIBER_STATUS_FINISHED || scheduler->status == ZEND_FIBER_STATUS_DEAD)) {
		zend_throw_error(zend_ce_fiber_error, "FiberScheduler::run() returned unexpectedly");
		return;
	}

	if (fiber->continuation->error == NULL) {
		RETVAL_COPY_VALUE(&fiber->continuation->value);
		ZVAL_UNDEF(&fiber->continuation->value);
		return;
	}

	error = fiber->continuation->error;
	fiber->continuation->error = NULL;
	exec = EG(current_execute_data);

	exec->opline--;
	zend_throw_exception_object(error);
	exec->opline++;
}
/* }}} */


/* {{{ proto Fiber::__wakeup() */
ZEND_METHOD(Fiber, __wakeup)
{
	/* Just specifying the zend_class_unserialize_deny handler is not enough,
	 * because it is only invoked for C unserialization. For O the error has
	 * to be thrown in __wakeup. */

	ZEND_PARSE_PARAMETERS_NONE();

	zend_throw_error(zend_ce_fiber_error, "Unserialization of 'Fiber' is not allowed");
}
/* }}} */


/* {{{ proto FiberError::__construct(string $message) */
ZEND_METHOD(FiberError, __construct)
{
	zend_throw_error(NULL, "FiberError cannot be constructed manually");
}
/* }}} */


ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fiber_run, 0, 0, IS_VOID, 0)
	ZEND_ARG_CALLABLE_INFO(0, callable, 0)
	ZEND_ARG_VARIADIC_INFO(0, arguments)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_fiber_continue, 0, 0, 2)
	 ZEND_ARG_OBJ_INFO(0, exception, Throwable, 1)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_fiber_await, 0, 0, 2)
	ZEND_ARG_OBJ_INFO(0, awaitable, Awaitable, 0)
	ZEND_ARG_OBJ_INFO(0, scheduler, FiberScheduler, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fiber_void, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry fiber_methods[] = {
	ZEND_ME(Fiber, run, arginfo_fiber_run, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME(Fiber, continue, arginfo_fiber_continue, ZEND_ACC_PRIVATE)
	ZEND_ME(Fiber, await, arginfo_fiber_await, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME(Fiber, __wakeup, arginfo_fiber_void, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_awaitable_onResolve, 0, 1, IS_VOID, 0)
	ZEND_ARG_CALLABLE_INFO(0, onResolve, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry awaitable_methods[] = {
	ZEND_ABSTRACT_ME(Awaitable, onResolve, arginfo_awaitable_onResolve)
	ZEND_FE_END
};

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_scheduler_run, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry scheduler_methods[] = {
	ZEND_ABSTRACT_ME(FiberScheduler, run, arginfo_scheduler_run)
	ZEND_FE_END
};

ZEND_BEGIN_ARG_INFO(arginfo_fiber_error_create, 0)
	ZEND_ARG_TYPE_INFO(0, message, IS_STRING, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry fiber_error_methods[] = {
	ZEND_ME(FiberError, __construct, arginfo_fiber_error_create, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	ZEND_FE_END
};

void zend_fiber_ce_register()
{
	zend_class_entry ce;
	zend_uchar opcode = ZEND_VM_LAST_OPCODE + 1;

	/* Create a new user opcode to run fiber. */
	while (1) {
		if (opcode == 255) {
			return;
		} else if (zend_get_user_opcode_handler(opcode) == NULL) {
			break;
		}
		opcode++;
	}

	zend_set_user_opcode_handler(opcode, fiber_run_opcode_handler);

	ZEND_SECURE_ZERO(fiber_run_op, sizeof(fiber_run_op));
	fiber_run_op[0].opcode = opcode;
	zend_vm_set_opcode_handler_ex(fiber_run_op, 0, 0, 0);
	fiber_run_op[1].opcode = opcode;
	zend_vm_set_opcode_handler_ex(fiber_run_op + 1, 0, 0, 0);

	ZEND_SECURE_ZERO(&fiber_run_func, sizeof(fiber_run_func));
	fiber_run_func.type = ZEND_USER_FUNCTION;
	fiber_run_func.function_name = zend_string_init("Fiber::run", sizeof("Fiber::run") - 1, 1);
	fiber_run_func.filename = ZSTR_EMPTY_ALLOC();
	fiber_run_func.opcodes = fiber_run_op;
	fiber_run_func.last_try_catch = 1;
	fiber_run_func.try_catch_array = &fiber_terminate_try_catch_array;

	INIT_CLASS_ENTRY(ce, "Fiber", fiber_methods);
	zend_ce_fiber = zend_register_internal_class(&ce);
	zend_ce_fiber->ce_flags |= ZEND_ACC_FINAL;
	zend_ce_fiber->create_object = zend_fiber_object_create;
	zend_ce_fiber->serialize = zend_class_serialize_deny;
	zend_ce_fiber->unserialize = zend_class_unserialize_deny;

	zend_fiber_handlers = std_object_handlers;
	zend_fiber_handlers.free_obj = zend_fiber_object_destroy;
	zend_fiber_handlers.clone_obj = NULL;
	zend_fiber_handlers.get_constructor = zend_fiber_get_constructor;

	INIT_CLASS_ENTRY(ce, "Awaitable", awaitable_methods);
	zend_ce_awaitable = zend_register_internal_interface(&ce);

	INIT_CLASS_ENTRY(ce, "FiberScheduler", scheduler_methods);
	zend_ce_fiber_scheduler = zend_register_internal_interface(&ce);

	INIT_CLASS_ENTRY(ce, "FiberError", fiber_error_methods);
	zend_ce_fiber_error = zend_register_internal_class_ex(&ce, zend_ce_error);
	zend_ce_fiber_error->ce_flags |= ZEND_ACC_FINAL;
	zend_ce_fiber_error->create_object = zend_ce_error->create_object;

	zend_hash_init(&fibers, 0, NULL, zend_fiber_hash_index_destroy, 1);
	zend_hash_init(&schedulers, 0, NULL, NULL, 1);
	
	scheduler_run_name = zend_string_init("run", sizeof("run") - 1, 1);
	fiber_continue_name = zend_string_init("continue", sizeof("continue") - 1, 1);
}

void zend_fiber_ce_unregister()
{
	zend_string_free(fiber_run_func.function_name);
	fiber_run_func.function_name = NULL;

	zend_hash_destroy(&fibers);
	zend_hash_destroy(&schedulers);
	
	zend_string_free(scheduler_run_name);
	scheduler_run_name = NULL;
	
	zend_string_free(fiber_continue_name);
	fiber_continue_name = NULL;
}

void zend_fiber_shutdown()
{
	zend_fiber *fiber;

	zend_fiber_cleanup();

	fiber = FIBER_G(root_fiber);

	FIBER_G(root_fiber) = NULL;
	FIBER_G(current_fiber) = NULL;

	if (fiber == NULL) {
		return;
	}

	zval_ptr_dtor(&fiber->continuation->value);
	zval_ptr_dtor(&fiber->continuation->closure);
}
