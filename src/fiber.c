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
#include "zend_builtin_functions.h"

#include "php_fiber.h"
#include "fiber.h"

PHP_FIBER_API zend_class_entry *zend_ce_fiber;
PHP_FIBER_API zend_class_entry *zend_ce_fiber_scheduler;

static zend_class_entry *zend_ce_reflection_fiber;
static zend_class_entry *zend_ce_reflection_fiber_scheduler;

static zend_class_entry *zend_ce_fiber_error;
static zend_class_entry *zend_ce_fiber_exit;

static zend_object_handlers zend_fiber_handlers;
static zend_object_handlers zend_scheduler_fiber_handlers;
static zend_object_handlers zend_reflection_fiber_handlers;
static zend_object_handlers zend_reflection_fiber_scheduler_handlers;

static zend_fiber *zend_fiber_create_root();

static zend_object *zend_fiber_object_create(zend_class_entry *ce);
static void zend_fiber_object_destroy(zend_object *object);

static zend_op_array fiber_run_func;
static zend_try_catch_element fiber_terminate_try_catch_array = { 0, 1, 0, 0 };
static zend_op fiber_run_op[2];

static zend_string *scheduler_run_name;

#define ZEND_FIBER_BACKUP_EG(stack, stack_page_size, execute_data, trace_num) do { \
	stack = EG(vm_stack); \
	stack->top = EG(vm_stack_top); \
	stack->end = EG(vm_stack_end); \
	stack_page_size = EG(vm_stack_page_size); \
	execute_data = EG(current_execute_data); \
	trace_num = EG(jit_trace_num); \
} while (0)

#define ZEND_FIBER_RESTORE_EG(stack, stack_page_size, execute_data, trace_num) do { \
	EG(vm_stack) = stack; \
	EG(vm_stack_top) = stack->top; \
	EG(vm_stack_end) = stack->end; \
	EG(vm_stack_page_size) = stack_page_size; \
	EG(current_execute_data) = execute_data; \
	EG(jit_trace_num) = trace_num; \
} while (0)


zend_always_inline PHP_FIBER_API zend_bool zend_fiber_is_scheduler(zend_fiber *fiber)
{
	return fiber->value == NULL;
}


PHP_FIBER_API zend_fiber *zend_get_root_fiber()
{
	if (FIBER_G(root_fiber) == NULL) {
		FIBER_G(root_fiber) = zend_fiber_create_root();
		FIBER_G(current_fiber) = FIBER_G(root_fiber);
	}

	return FIBER_G(root_fiber);
}


PHP_FIBER_API zend_fiber *zend_get_current_fiber()
{
	if (FIBER_G(current_fiber) == NULL) {
		return zend_get_root_fiber();
	}

	return FIBER_G(current_fiber);
}


zend_always_inline PHP_FIBER_API zend_long zend_fiber_get_id(zend_fiber *fiber)
{
	return fiber->id;
}


zend_always_inline PHP_FIBER_API zend_long zend_fiber_get_current_id()
{
	return zend_get_current_fiber()->id;
}


zend_always_inline PHP_FIBER_API zend_bool zend_is_fiber_exit(zend_object *exception)
{
	ZEND_ASSERT(exception && "No exception object provided");

	return exception->ce == zend_ce_fiber_exit;
}


static zend_fiber *zend_fiber_create_root()
{
	zend_fiber *root_fiber;

	ZEND_ASSERT(FIBER_G(root_fiber) == NULL);

	root_fiber = (zend_fiber *) zend_fiber_object_create(zend_ce_fiber);
	root_fiber->context = zend_fiber_create_root_context();
	root_fiber->execute_data = EG(current_execute_data);

	root_fiber->status = ZEND_FIBER_STATUS_RUNNING;

	FIBER_G(root_fiber) = root_fiber;

	// Add a second reference to prevent garbage collection of the root fiber.
	GC_ADDREF(&root_fiber->std);

	return root_fiber;
}


static zend_bool zend_fiber_switch_to(zend_fiber *fiber)
{
	zend_fiber *previous;
	zend_vm_stack stack;
	size_t stack_page_size;
	zend_execute_data *execute_data;
	uint32_t jit_trace_num;
	zend_bool result;

	previous = FIBER_G(current_fiber);

	ZEND_ASSERT(previous != NULL);

	FIBER_G(current_fiber) = fiber;

	ZEND_FIBER_BACKUP_EG(stack, stack_page_size, execute_data, jit_trace_num);

	result = zend_fiber_switch_context(previous->context, fiber->context);

	FIBER_G(current_fiber) = previous;

	ZEND_FIBER_RESTORE_EG(stack, stack_page_size, execute_data, jit_trace_num);

	return result;
}


static zend_bool zend_fiber_suspend(zend_fiber *fiber)
{
	zend_vm_stack stack;
	size_t stack_page_size;
	zend_execute_data *execute_data;
	uint32_t jit_trace_num;
	zend_bool result;

	ZEND_ASSERT(fiber != FIBER_G(root_fiber)); // Root fiber should not be suspended.

	ZEND_FIBER_BACKUP_EG(stack, stack_page_size, execute_data, jit_trace_num);

	result = zend_fiber_suspend_context(fiber->context);

	ZEND_FIBER_RESTORE_EG(stack, stack_page_size, execute_data, jit_trace_num);

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


static void zend_fiber_run()
{
	zend_fiber *fiber;

	fiber = FIBER_G(current_fiber);
	ZEND_ASSERT(fiber != NULL);

	fiber->execute_data = (zend_execute_data *) fiber->stack->top;
	EG(vm_stack) = fiber->stack;
	EG(vm_stack_top) = (zval *) fiber->stack->top + ZEND_CALL_FRAME_SLOT;
	EG(vm_stack_end) = fiber->stack->end;
	EG(vm_stack_page_size) = ZEND_FIBER_VM_STACK_SIZE;

	zend_vm_init_call_frame(fiber->execute_data, ZEND_CALL_TOP_FUNCTION, (zend_function *) &fiber_run_func, 0, NULL);

	fiber->execute_data->opline = fiber_run_op;
	fiber->execute_data->call = NULL;
	fiber->execute_data->return_value = NULL;
	fiber->execute_data->prev_execute_data = NULL;

	EG(current_execute_data) = fiber->execute_data;
	EG(jit_trace_num) = 0;

	zend_execute_ex(fiber->execute_data);

	zend_vm_stack_destroy();
	fiber->stack = NULL;
	fiber->execute_data = NULL;

	zend_fiber_suspend_context(fiber->context);

	abort();
}


static int fiber_run_opcode_handler(zend_execute_data *execute_data)
{
	zend_fiber *fiber;
	zval retval;

	fiber = FIBER_G(current_fiber);
	ZEND_ASSERT(fiber != NULL && fiber != FIBER_G(root_fiber));
	ZEND_ASSERT(fiber->execute_data == execute_data && "Fiber execute data corrupted");

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

	if (zend_fiber_is_scheduler(fiber)) {
		// Scheduler fibers retain reference to closure while running.
		zval_ptr_dtor(&fiber->fci.function_name);
	}

	GC_DELREF(&fiber->std);

	zval_ptr_dtor(&retval);

	return ZEND_USER_OPCODE_RETURN;
}


static zend_bool zend_fiber_resume(zend_fiber *fiber, zend_fiber *scheduler)
{
	ZEND_ASSERT(!zend_fiber_is_scheduler(fiber) && zend_fiber_is_scheduler(scheduler));

	if (scheduler->link == fiber) {
		// Suspend from scheduler to fiber that resumed the scheduler.
		scheduler->status = ZEND_FIBER_STATUS_SUSPENDED;
		scheduler->link = NULL;
		return zend_fiber_suspend(scheduler);
	}

	fiber->link = NULL;

	// Another fiber started the scheduler, so switch to resuming fiber.
	return zend_fiber_switch_to(fiber);
}


static void zend_fiber_free(zend_fiber *fiber)
{
	if (fiber->status == ZEND_FIBER_STATUS_SUSPENDED) {
		fiber->status = ZEND_FIBER_STATUS_SHUTDOWN;
		zend_fiber_switch_to(fiber);
	} else if (fiber->status == ZEND_FIBER_STATUS_INIT) {
		zval_ptr_dtor(&fiber->fci.function_name);
		efree(fiber->stack);
	}

	zend_fiber_destroy(fiber->context);

	zend_object_std_dtor(&fiber->std);
}


static zend_object *zend_fiber_object_create(zend_class_entry *ce)
{
	zend_fiber *fiber;

	fiber = emalloc(sizeof(zend_fiber) + sizeof(zval));
	memset(fiber, 0, sizeof(zend_fiber) + sizeof(zval));

	fiber->id = FIBER_G(id)++;

	zend_object_std_init(&fiber->std, ce);
	fiber->std.handlers = &zend_fiber_handlers;

	ZVAL_UNDEF(&fiber->fci.function_name);

	fiber->value = (zval *) ((char *) fiber + sizeof(zend_fiber));
	ZVAL_UNDEF(fiber->value);

	zend_hash_index_add_ptr(&FIBER_G(fibers), fiber->std.handle, fiber);

	return &fiber->std;
}


static void zend_fiber_object_destroy(zend_object *object)
{
	zend_fiber *fiber = (zend_fiber *) object;

	ZEND_ASSERT(!zend_fiber_is_scheduler(fiber));

	zval_ptr_dtor(fiber->value);

	zend_hash_index_del(&FIBER_G(fibers), fiber->std.handle);

	zend_fiber_free(fiber);
}


static zend_object *zend_scheduler_fiber_object_create(zend_class_entry *ce)
{
	zend_fiber *fiber;

	fiber = emalloc(sizeof(zend_fiber));
	memset(fiber, 0, sizeof(zend_fiber));

	fiber->id = FIBER_G(id)++;

	zend_object_std_init(&fiber->std, ce);
	fiber->std.handlers = &zend_scheduler_fiber_handlers;

	ZVAL_UNDEF(&fiber->fci.function_name);

	return &fiber->std;
}


static void zend_scheduler_fiber_object_destroy(zend_object *object)
{
	zend_fiber *fiber = (zend_fiber *) object;

	ZEND_ASSERT(zend_fiber_is_scheduler(fiber));

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


static zend_fiber *zend_fiber_create_from_scheduler(zval *scheduler)
{
	zend_fiber *fiber;
	zend_function *func;
	zval context;

	ZEND_ASSERT(instanceof_function(Z_OBJCE_P(scheduler), zend_ce_fiber_scheduler));

	fiber = (zend_fiber *) zend_scheduler_fiber_object_create(zend_ce_fiber);

	func = zend_hash_find_ptr(&(Z_OBJCE_P(scheduler)->function_table), scheduler_run_name);
	zend_create_fake_closure(&context, func, func->op_array.scope, Z_OBJCE_P(scheduler), scheduler);

	if (zend_fcall_info_init(&context, 0, &fiber->fci, &fiber->fci_cache, NULL, NULL) == FAILURE) {
		efree(fiber);
		zval_ptr_dtor(&context);
		zend_throw_error(NULL, "Failed initializing %s::run() fcall info", Z_OBJCE_P(scheduler)->name->val);
		return NULL;
	}

	// Keep a reference to closure as long as fiber is running.
	Z_TRY_ADDREF(fiber->fci.function_name);

	zval_ptr_dtor(&context);

	fiber->context = zend_fiber_create_context();
	fiber->stack_size = FIBER_G(stack_size);

	if (fiber->context == NULL) {
		efree(fiber);
		zend_throw_error(zend_ce_fiber_exit, "Failed creating scheduler fiber context");
		return NULL;
	}

	if (!zend_fiber_create(fiber->context, zend_fiber_run, fiber->stack_size)) {
		zend_fiber_destroy(fiber->context);
		efree(fiber);
		zend_throw_error(zend_ce_fiber_exit, "Failed creating scheduler fiber");
		return NULL;
	}

	fiber->stack = zend_fiber_vm_stack_alloc(ZEND_FIBER_VM_STACK_SIZE);

	fiber->status = ZEND_FIBER_STATUS_INIT;

	// Assign object to zval for proper GC.
	ZVAL_OBJ(&context, &fiber->std);
	Z_ADDREF(context);
	zval_ptr_dtor(&context);

	return fiber;
}


static zend_fiber *zend_fiber_get_scheduler(zval *scheduler)
{
	zend_fiber *fiber;
	zend_ulong handle = Z_OBJ_HANDLE_P(scheduler);

	ZEND_ASSERT(instanceof_function(Z_OBJCE_P(scheduler), zend_ce_fiber_scheduler));

	fiber = zend_hash_index_find_ptr(&FIBER_G(schedulers), handle);

	if (fiber != NULL) {
		if (EXPECTED(!(fiber->status & ZEND_FIBER_STATUS_FINISHED))) {
			return fiber;
		}

		zend_hash_index_del(&FIBER_G(schedulers), handle);
	}

	fiber = zend_fiber_create_from_scheduler(scheduler);

	if (fiber == NULL) {
		// FiberError thrown in zend_fiber_create_from_scheduler.
		return NULL;
	}

	zend_hash_index_add_ptr(&FIBER_G(schedulers), handle, fiber);
	GC_ADDREF(&fiber->std);

	return fiber;
}


void zend_fiber_scheduler_hash_index_dtor(zval *ptr)
{
	zend_fiber *fiber = Z_PTR_P(ptr);
	GC_DELREF(&fiber->std);
}


static void zend_fiber_clean_shutdown()
{
	zend_fiber *fiber;
	uint32_t handle;

	ZEND_HASH_REVERSE_FOREACH_PTR(&FIBER_G(fibers), fiber) {
		if (fiber->status == ZEND_FIBER_STATUS_SUSPENDED) {
			fiber->status = ZEND_FIBER_STATUS_SHUTDOWN;
			GC_ADDREF(&fiber->std);
			zend_fiber_switch_to(fiber);
		}
	} ZEND_HASH_FOREACH_END();

	ZEND_HASH_REVERSE_FOREACH_NUM_KEY_PTR(&FIBER_G(schedulers), handle, fiber) {
		if (fiber->status == ZEND_FIBER_STATUS_SUSPENDED) {
			fiber->status = ZEND_FIBER_STATUS_SHUTDOWN;
			zend_fiber_switch_to(fiber);
		}

		zend_hash_index_del(&FIBER_G(schedulers), handle);
	} ZEND_HASH_FOREACH_END();
}


static void zend_fiber_shutdown_cleanup()
{
	zend_fiber *fiber;
	uint32_t handle;

	ZEND_HASH_REVERSE_FOREACH_PTR(&FIBER_G(fibers), fiber) {
		if (fiber->status == ZEND_FIBER_STATUS_SUSPENDED) {
			fiber->status = ZEND_FIBER_STATUS_SHUTDOWN;
			GC_ADDREF(&fiber->std);
		}
	} ZEND_HASH_FOREACH_END();

	ZEND_HASH_REVERSE_FOREACH_NUM_KEY_PTR(&FIBER_G(schedulers), handle, fiber) {
		zend_hash_index_del(&FIBER_G(schedulers), handle);
	} ZEND_HASH_FOREACH_END();
}


static void zend_fiber_observer_end(zend_execute_data *execute_data, zval *retval)
{
	zend_fiber *fiber;
	zval exception;

	if (FIBER_G(shutdown)) {
		return;
	}

	ZVAL_UNDEF(&exception);

	if (EG(exception)) {
		if (!zend_is_unwind_exit(EG(exception))) {
			ZVAL_OBJ(&exception, EG(exception));
			Z_ADDREF(exception);
			zend_clear_exception();
		}
	} else {
		while (zend_array_count(&FIBER_G(schedulers))) {
			uint32_t handle;

			ZEND_HASH_REVERSE_FOREACH_NUM_KEY_PTR(&FIBER_G(schedulers), handle, fiber) {
				if (fiber->status == ZEND_FIBER_STATUS_SUSPENDED) {
					fiber->status = ZEND_FIBER_STATUS_RUNNING;
					zend_fiber_switch_to(fiber);
				}

				zend_hash_index_del(&FIBER_G(schedulers), handle);

				if (EG(exception)) {
					ZVAL_OBJ(&exception, EG(exception));
					Z_ADDREF(exception);
					zend_clear_exception();
					goto shutdown;
				}
			} ZEND_HASH_FOREACH_END();
		}
	}

	shutdown: {
		zend_fiber_clean_shutdown();

		if (Z_TYPE(exception) == IS_OBJECT) {
			zend_throw_exception_object(&exception);
		}
	}
}


zend_observer_fcall_handlers zend_fiber_observer_fcall_init(zend_execute_data *execute_data)
{
	if (!execute_data->func->common.function_name && !execute_data->prev_execute_data) {
		return (zend_observer_fcall_handlers){NULL, zend_fiber_observer_end};
	}

	return (zend_observer_fcall_handlers){NULL, NULL};
}


void zend_fiber_error_observer(int type, const char *filename, uint32_t line, zend_string *message)
{
	if (!(type & E_FATAL_ERRORS)) {
		return; // Non-fatal error, nothing to do.
	}

	if (FIBER_G(shutdown)) {
		return; // Already shut down, nothing to do.
	}

	// Fatal error, mark as shutdown so schedulers are not continued on exit.
	FIBER_G(shutdown) = 1;

	if (type & E_DONT_BAIL) {
		// Uncaught exception, do a clean shutdown now.
		zend_fiber_clean_shutdown();
	}
}


static ZEND_COLD void zend_fiber_scheduler_uncaught_exception_handler(zval *scheduler, zend_object *exception)
{
	zval retval;

	ZEND_ASSERT(instanceof_function(Z_OBJCE_P(scheduler), zend_ce_fiber_scheduler));
	ZEND_ASSERT(exception && "Exception undefined");

	if (zend_is_unwind_exit(exception) || zend_is_fiber_exit(exception)) {
		return; // Exception is UnwindExit or FiberExit, so ignore as we are exiting anyway.
	}

	zend_throw_error(
		zend_ce_fiber_exit,
		"Uncaught %s thrown from %s::run(): %s",
		exception->ce->name->val,
		Z_OBJCE_P(scheduler)->name->val,
		Z_STRVAL_P(zend_read_property(exception->ce, exception, "message", sizeof("message") - 1, 0, &retval))
	);
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


static void zend_reflection_fiber_object_destroy(zend_object *object)
{
	zend_fiber_reflection *reflection = (zend_fiber_reflection *) object;

	if (reflection->fiber != NULL) {
		GC_DELREF(&reflection->fiber->std);
	}

	zend_object_std_dtor(&reflection->std);
}


static zend_object *zend_reflection_fiber_scheduler_object_create(zend_class_entry *ce)
{
	zend_fiber_reflection *reflection;

	reflection = emalloc(sizeof(zend_fiber_reflection));
	memset(reflection, 0, sizeof(zend_fiber_reflection));

	zend_object_std_init(&reflection->std, ce);
	reflection->std.handlers = &zend_reflection_fiber_scheduler_handlers;

	return &reflection->std;
}


static void zend_reflection_fiber_scheduler_object_destroy(zend_object *object)
{
	zend_fiber_reflection *reflection = (zend_fiber_reflection *) object;

	if (reflection->fiber != NULL) {
		Z_DELREF(reflection->fiber->fci.function_name);
		GC_DELREF(&reflection->fiber->std);
	}

	zend_object_std_dtor(&reflection->std);
}


/* {{{ proto Fiber Fiber::this() */
ZEND_METHOD(Fiber, this)
{
	zend_fiber *fiber;

	ZEND_PARSE_PARAMETERS_NONE();

	fiber = FIBER_G(current_fiber);

	if (fiber == NULL) {
		fiber = zend_fiber_create_root();

		if (UNEXPECTED(fiber == NULL)) {
			zend_throw_error(zend_ce_fiber_exit, "Could not create root fiber context");
			return;
		}

		FIBER_G(current_fiber) = fiber;
	}

	if (zend_fiber_is_scheduler(fiber)) {
		zend_throw_error(zend_ce_fiber_error, "Cannot call Fiber::this() within a fiber scheduler");
		return;
	}

	RETURN_OBJ_COPY(&fiber->std);
}
/* }}} */


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
	zend_fiber *fiber, *scheduler;
	zval context, *params;
	uint32_t param_count;

	if (UNEXPECTED(FIBER_G(shutdown))) {
		zend_throw_error(zend_ce_fiber_error, "Cannot start a fiber during shutdown");
		return;
	}

	scheduler = FIBER_G(current_fiber);

	if (scheduler == NULL || !zend_fiber_is_scheduler(scheduler)) {
		zend_throw_error(zend_ce_fiber_error, "New fibers can only be started within a scheduler");
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

	fiber->context = zend_fiber_create_context();
	fiber->stack_size = FIBER_G(stack_size);

	if (fiber->context == NULL) {
		zval_ptr_dtor(&context);
		zend_throw_error(zend_ce_fiber_exit, "Failed to create native fiber context");
		return;
	}

	if (!zend_fiber_create(fiber->context, zend_fiber_run, fiber->stack_size)) {
		zval_ptr_dtor(&context);
		zend_throw_error(zend_ce_fiber_exit, "Failed to create native fiber");
		return;
	}

	fiber->stack = zend_fiber_vm_stack_alloc(ZEND_FIBER_VM_STACK_SIZE);

	fiber->status = ZEND_FIBER_STATUS_RUNNING;

	scheduler->execute_data = execute_data;

	GC_ADDREF(&fiber->std);

	if (!zend_fiber_switch_to(fiber)) {
		fiber->status = ZEND_FIBER_STATUS_INIT;
		GC_DELREF(&fiber->std);
		zend_throw_error(zend_ce_fiber_exit, "Failed switching to fiber");
		return;
	}

	zval_ptr_dtor(&fiber->fci.function_name);
}
/* }}} */


/* {{{ proto mixed Fiber::suspend(FiberScheduler $scheduler) */
ZEND_METHOD(Fiber, suspend)
{
	zend_fiber *fiber, *scheduler;
	zval *fiber_scheduler, *error;

	if (UNEXPECTED(FIBER_G(shutdown))) {
		zend_throw_error(zend_ce_fiber_error, "Cannot suspend during shutdown");
		return;
	}

	fiber = FIBER_G(current_fiber);

	if (UNEXPECTED(fiber == NULL)) {
		zend_throw_error(zend_ce_fiber_error, "Main fiber suspended without path to resuming");
		return;
	}

	if (UNEXPECTED(zend_fiber_is_scheduler(fiber))) {
		zend_throw_error(zend_ce_fiber_error, "Cannot suspend in a scheduler");
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

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS(fiber_scheduler, zend_ce_fiber_scheduler)
	ZEND_PARSE_PARAMETERS_END();

	scheduler = zend_fiber_get_scheduler(fiber_scheduler);

	if (scheduler == NULL) {
		// FiberError thrown in zend_fiber_get_scheduler.
		return;
	}

	fiber->execute_data = execute_data;

	fiber->status = ZEND_FIBER_STATUS_SUSPENDED;

	GC_DELREF(&fiber->std);
	fiber->link = scheduler;

	if (scheduler->status == ZEND_FIBER_STATUS_RUNNING) {
		if (!zend_fiber_suspend(fiber)) {
			fiber->status = ZEND_FIBER_STATUS_RUNNING;
			zend_throw_error(zend_ce_fiber_exit, "Failed suspending fiber");
			return;
		}
	} else {
		scheduler->status = ZEND_FIBER_STATUS_RUNNING;
		scheduler->link = fiber;

		if (!zend_fiber_switch_to(scheduler)) {
			fiber->status = ZEND_FIBER_STATUS_RUNNING;
			zend_throw_error(zend_ce_fiber_exit, "Failed switching to scheduler");
			return;
		}
	}

	if (fiber->status == ZEND_FIBER_STATUS_SHUTDOWN) {
		// This occurs on exit if the fiber never resumed, it has been GC'ed, so do not add a ref.
		zend_throw_error(zend_ce_fiber_exit, "Fiber destroyed");
		return;
	}

	fiber->status = ZEND_FIBER_STATUS_RUNNING;
	GC_ADDREF(&fiber->std);
	
	if (UNEXPECTED(EG(exception))) {
		// Exception thrown from scheduler, invoke exception handler and bailout.
		zend_fiber_scheduler_uncaught_exception_handler(fiber_scheduler, EG(exception));
		return;
	}

	if (UNEXPECTED(scheduler->status & ZEND_FIBER_STATUS_FINISHED)) {
		zend_throw_error(
			zend_ce_fiber_error,
			"%s::run() returned before resuming the fiber",
			Z_OBJCE_P(fiber_scheduler)->name->val
		);
		return;
	}

	if (fiber->error == NULL) {
		RETVAL_COPY_VALUE(fiber->value);
		ZVAL_UNDEF(fiber->value);
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
	zend_fiber *fiber, *scheduler;
	zval *value = NULL;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 0, 1)
		Z_PARAM_OPTIONAL
		Z_PARAM_ZVAL(value);
	ZEND_PARSE_PARAMETERS_END();

	scheduler = FIBER_G(current_fiber);

	if (UNEXPECTED(scheduler == NULL || !zend_fiber_is_scheduler(scheduler))) {
		zend_throw_error(zend_ce_fiber_error, "Fibers can only be resumed within a scheduler");
		return;
	}

	fiber = (zend_fiber *) Z_OBJ_P(getThis());

	if (UNEXPECTED(fiber->status != ZEND_FIBER_STATUS_SUSPENDED)) {
		zend_throw_error(zend_ce_fiber_error, "Cannot resume a fiber that is not suspended");
		return;
	}

	if (value != NULL) {
		ZVAL_COPY(fiber->value, value);
	} else {
		ZVAL_NULL(fiber->value);
	}

	fiber->status = ZEND_FIBER_STATUS_RUNNING;

	if (UNEXPECTED(fiber->link != NULL && fiber->link != scheduler)) {
		zend_throw_error(zend_ce_fiber_exit, "Fiber resumed by a scheduler other than that provided to Fiber::suspend()");
		return;
	}

	fiber->link = NULL;

	scheduler->execute_data = execute_data;

	if (!zend_fiber_resume(fiber, scheduler)) {
		fiber->status = ZEND_FIBER_STATUS_SUSPENDED;
		zend_throw_error(zend_ce_fiber_exit, "Failed resuming fiber");
		return;
	}

	if (scheduler->status == ZEND_FIBER_STATUS_SHUTDOWN) {
		zend_throw_error(zend_ce_fiber_exit, "Fiber destroyed");
		return;
	}

	scheduler->status = ZEND_FIBER_STATUS_RUNNING;
}
/* }}} */


/* {{{ proto void Fiber::throw(Throwable $exception) */
ZEND_METHOD(Fiber, throw)
{
	zend_fiber *fiber, *scheduler;
	zval *exception;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS_EX(exception, zend_ce_throwable, 0, 0)
	ZEND_PARSE_PARAMETERS_END();

	scheduler = FIBER_G(current_fiber);

	if (UNEXPECTED(scheduler == NULL || !zend_fiber_is_scheduler(scheduler))) {
		zend_throw_error(zend_ce_fiber_error, "Fibers can only be resumed within a scheduler");
		return;
	}

	fiber = (zend_fiber *) Z_OBJ_P(getThis());

	if (UNEXPECTED(fiber->status != ZEND_FIBER_STATUS_SUSPENDED)) {
		zend_throw_error(zend_ce_fiber_error, "Cannot resume a fiber that is not suspended");
		return;
	}

	Z_ADDREF_P(exception);
	fiber->error = exception;

	fiber->status = ZEND_FIBER_STATUS_RUNNING;

	if (UNEXPECTED(fiber->link != NULL && fiber->link != scheduler)) {
		zend_throw_error(zend_ce_fiber_exit, "Fiber resumed by a scheduler other than that provided to Fiber::suspend()");
		return;
	}

	fiber->link = NULL;

	scheduler->execute_data = execute_data;

	if (!zend_fiber_resume(fiber, scheduler)) {
		fiber->status = ZEND_FIBER_STATUS_SUSPENDED;
		zend_throw_error(zend_ce_fiber_exit, "Failed resuming fiber");
		return;
	}

	if (scheduler->status == ZEND_FIBER_STATUS_SHUTDOWN) {
		zend_throw_error(zend_ce_fiber_exit, "Fiber destroyed");
		return;
	}

	scheduler->status = ZEND_FIBER_STATUS_RUNNING;
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


/* {{{ proto ReflectionFiberScheduler::__construct(FiberScheduler $scheduler) */
ZEND_METHOD(ReflectionFiberScheduler, __construct)
{
	zend_fiber_reflection *reflection;
	zend_fiber *fiber;
	zval *scheduler;

	ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
		Z_PARAM_OBJECT_OF_CLASS_EX(scheduler, zend_ce_fiber_scheduler, 0, 0)
	ZEND_PARSE_PARAMETERS_END();

	fiber = zend_fiber_get_scheduler(scheduler);

	if (fiber == NULL) {
		// FiberError thrown in zend_fiber_get_scheduler.
		return;
	}

	reflection = (zend_fiber_reflection *) Z_OBJ_P(getThis());
	reflection->fiber = fiber;

	GC_ADDREF(&reflection->fiber->std);
	Z_ADDREF(reflection->fiber->fci.function_name);
}
/* }}} */


/* {{{ proto FiberScheduler ReflectionFiberScheduler::getScheduler() */
ZEND_METHOD(ReflectionFiberScheduler, getScheduler)
{
	zend_fiber_reflection *reflection;

	ZEND_PARSE_PARAMETERS_NONE();

	reflection = (zend_fiber_reflection *) Z_OBJ_P(getThis());

	RETURN_COPY(zend_get_closure_this_ptr(&reflection->fiber->fci.function_name));
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

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_fiber_this, 0, 0, Fiber, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fiber_start, 0, 0, IS_VOID, 0)
	ZEND_ARG_VARIADIC_INFO(0, arguments)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fiber_resume, 0, 0, IS_VOID, 0)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fiber_throw, 0, 1, IS_VOID, 0)
	ZEND_ARG_OBJ_INFO(0, exception, Throwable, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fiber_status, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_fiber_suspend, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, scheduler, FiberScheduler, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fiber_void, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry fiber_methods[] = {
	ZEND_ME(Fiber, __construct, arginfo_fiber_construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	ZEND_ME(Fiber, this, arginfo_fiber_this, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME(Fiber, start, arginfo_fiber_start, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, resume, arginfo_fiber_resume, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, throw, arginfo_fiber_throw, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, isStarted, arginfo_fiber_status, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, isSuspended, arginfo_fiber_status, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, isRunning, arginfo_fiber_status, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, isTerminated, arginfo_fiber_status, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, suspend, arginfo_fiber_suspend, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME(Fiber, __wakeup, arginfo_fiber_void, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_scheduler_run, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry scheduler_methods[] = {
	ZEND_ABSTRACT_ME(FiberScheduler, run, arginfo_scheduler_run)
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
	ZEND_ME(ReflectionFiber, getTrace, arginfo_reflection_fiber_getTrace, ZEND_ACC_PUBLIC)
	ZEND_ME(ReflectionFiber, getExecutingLine, arginfo_reflection_fiber_getExecutingLine, ZEND_ACC_PUBLIC)
	ZEND_ME(ReflectionFiber, getExecutingFile, arginfo_reflection_fiber_getExecutingFile, ZEND_ACC_PUBLIC)
	ZEND_ME(ReflectionFiber, isStarted, arginfo_reflection_fiber_status, ZEND_ACC_PUBLIC)
	ZEND_ME(ReflectionFiber, isSuspended, arginfo_reflection_fiber_status, ZEND_ACC_PUBLIC)
	ZEND_ME(ReflectionFiber, isRunning, arginfo_reflection_fiber_status, ZEND_ACC_PUBLIC)
	ZEND_ME(ReflectionFiber, isTerminated, arginfo_reflection_fiber_status, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

ZEND_BEGIN_ARG_INFO(arginfo_reflection_fiber_scheduler_construct, 0)
	ZEND_ARG_OBJ_INFO(0, scheduler, FiberScheduler, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO(arginfo_reflection_fiber_scheduler_getScheduler, FiberScheduler, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry reflection_fiber_scheduler_methods[] = {
	ZEND_ME(ReflectionFiberScheduler, __construct, arginfo_reflection_fiber_scheduler_construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	ZEND_ME(ReflectionFiberScheduler, getScheduler, arginfo_reflection_fiber_scheduler_getScheduler, ZEND_ACC_PUBLIC)
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

	ZEND_SECURE_ZERO(fiber_run_op, sizeof(zend_op) * 2);
	fiber_run_op[0].opcode = opcode;
	zend_vm_set_opcode_handler_ex(fiber_run_op, 0, 0, 0);
	fiber_run_op[1].opcode = opcode;
	zend_vm_set_opcode_handler_ex(fiber_run_op + 1, 0, 0, 0);

	ZEND_SECURE_ZERO(&fiber_run_func, sizeof(fiber_run_func));
	fiber_run_func.type = ZEND_USER_FUNCTION;
	fiber_run_func.function_name = zend_string_init("Fiber::run", sizeof("Fiber::run") - 1, 1);
	fiber_run_func.filename = zend_string_init("[fiber function]", sizeof("[fiber function]") - 1, 1);
	fiber_run_func.opcodes = fiber_run_op;
	fiber_run_func.last_try_catch = 1;
	fiber_run_func.try_catch_array = &fiber_terminate_try_catch_array;

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

	zend_scheduler_fiber_handlers = std_object_handlers;
	zend_scheduler_fiber_handlers.free_obj = zend_scheduler_fiber_object_destroy;
	zend_scheduler_fiber_handlers.clone_obj = NULL;

	INIT_CLASS_ENTRY(ce, "FiberScheduler", scheduler_methods);
	zend_ce_fiber_scheduler = zend_register_internal_interface(&ce);

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

	INIT_CLASS_ENTRY(ce, "ReflectionFiberScheduler", reflection_fiber_scheduler_methods);
	zend_ce_reflection_fiber_scheduler = zend_register_internal_class_ex(&ce, zend_ce_reflection_fiber);
	zend_ce_fiber_exit->ce_flags |= ZEND_ACC_FINAL;
	zend_ce_reflection_fiber_scheduler->create_object = zend_reflection_fiber_scheduler_object_create;
	zend_ce_reflection_fiber_scheduler->serialize = zend_class_serialize_deny;
	zend_ce_reflection_fiber_scheduler->unserialize = zend_class_unserialize_deny;

	zend_reflection_fiber_scheduler_handlers = std_object_handlers;
	zend_reflection_fiber_scheduler_handlers.free_obj = zend_reflection_fiber_scheduler_object_destroy;
	zend_reflection_fiber_scheduler_handlers.clone_obj = NULL;

	scheduler_run_name = zend_string_init("run", sizeof("run") - 1, 1);
}

void zend_fiber_ce_unregister()
{
	zend_set_user_opcode_handler(ZEND_CATCH, FIBER_G(catch_handler));

	zend_string_free(fiber_run_func.function_name);
	fiber_run_func.function_name = NULL;

	zend_string_free(fiber_run_func.filename);
	fiber_run_func.filename = NULL;

	zend_string_free(scheduler_run_name);
	scheduler_run_name = NULL;
}

void zend_fiber_shutdown()
{
	zend_fiber *fiber;

	if (!FIBER_G(shutdown)) {
		zend_fiber_clean_shutdown();
	}

	FIBER_G(shutdown) = 1;

	zend_fiber_shutdown_cleanup();

	fiber = FIBER_G(root_fiber);

	if (fiber != NULL) {
		/* Root fiber has two internal references to prevent
		 * garbage collection from the removal of a single reference. */
		GC_DELREF(&fiber->std);
		GC_DELREF(&fiber->std);
	}
}
