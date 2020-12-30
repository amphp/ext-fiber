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

#ifndef FIBER_H
#define FIBER_H

#include "php.h"
#include "zend_observer.h"

BEGIN_EXTERN_C()

void zend_fiber_ce_register();
void zend_fiber_ce_unregister();

void zend_fiber_shutdown();

#ifdef PHP_WIN32
# ifdef PHP_FIBER_EXPORTS
#  define PHP_FIBER_API __declspec(dllexport)
# else
#  define PHP_FIBER_API __declspec(dllimport)
# endif
#elif defined(__GNUC__) && __GNUC__ >= 4
# define PHP_FIBER_API __attribute__ ((visibility("default")))
#else
# define PHP_FIBER_API
#endif

extern PHP_FIBER_API zend_class_entry *zend_ce_fiber;
extern PHP_FIBER_API zend_class_entry *zend_ce_scheduler_fiber;

typedef void* zend_fiber_context;

typedef struct _zend_fiber zend_fiber;

struct _zend_fiber {
	/* Fiber PHP object handle. */
	zend_object std;

	/* Unique ID assigned to this fiber. Root fiber is always 0. */
	zend_long id;

	/* Status of the fiber, one of the ZEND_FIBER_STATUS_* constants. */
	zend_uchar status;
	
	/* Used to determine which fiber entered a scheduler and which scheduler should resume a fiber. */
	zend_fiber *link;

	/* Callback and info / cache to be used when fiber is started. */
	zend_fcall_info fci;
	zend_fcall_info_cache fci_cache;

	/* Fiber context of this fiber, will be created during call to start(). */
	zend_fiber_context context;

	/* Current Zend VM execute data being run by the fiber. */
	zend_execute_data *execute_data;

	/* VM stack being used by the fiber. */
	zend_vm_stack stack;

	/* Max size of the C stack being used by the fiber. */
	size_t stack_size;

	/* Exception to be thrown from Fiber::suspend(). Unused on scheduler fibers. */
	zval *error;

	/* Value to return from Fiber::suspend(). Pointer invalid on scheduler fibers. */
	zval *value;
};

typedef struct _zend_fiber_reflection zend_fiber_reflection;

struct _zend_fiber_reflection {
	/* ReflectionFiber PHP object handle. */
	zend_object std;

	/* Fiber being reflected. */
	zend_fiber *fiber;
};

PHP_FIBER_API zend_fiber *zend_get_root_fiber();
PHP_FIBER_API zend_fiber *zend_get_current_fiber();
PHP_FIBER_API zend_long zend_fiber_get_id(zend_fiber *fiber);
PHP_FIBER_API zend_long zend_fiber_get_current_id();
PHP_FIBER_API zend_bool zend_fiber_is_scheduler(zend_fiber *fiber);
PHP_FIBER_API zend_bool zend_is_fiber_exit(zend_object *exception);

static const zend_uchar ZEND_FIBER_STATUS_INIT = 0;
static const zend_uchar ZEND_FIBER_STATUS_SUSPENDED = 0x1;
static const zend_uchar ZEND_FIBER_STATUS_RUNNING = 0x2;
static const zend_uchar ZEND_FIBER_STATUS_RETURNED = 0x4;
static const zend_uchar ZEND_FIBER_STATUS_THREW = 0x8;
static const zend_uchar ZEND_FIBER_STATUS_SHUTDOWN = 0x10;

static const zend_uchar ZEND_FIBER_STATUS_FINISHED = 0x1c;

typedef void (* zend_fiber_func)();

char *zend_fiber_backend_info();

zend_fiber_context zend_fiber_create_root_context();
zend_fiber_context zend_fiber_create_context();

zend_bool zend_fiber_create(zend_fiber_context context, zend_fiber_func func, size_t stack_size);
void zend_fiber_destroy(zend_fiber_context context);

zend_bool zend_fiber_switch_context(zend_fiber_context current, zend_fiber_context next);
zend_bool zend_fiber_suspend_context(zend_fiber_context current);

zend_observer_fcall_handlers zend_fiber_observer_fcall_init(zend_execute_data *execute_data);
void zend_fiber_error_observer(int type, const char *filename, uint32_t line, zend_string *message);

void zend_fiber_scheduler_hash_index_dtor(zval *ptr);

END_EXTERN_C()

#define ZEND_FIBER_VM_STACK_SIZE 4096

#endif

/*
 * vim: sw=4 ts=4
 * vim600: fdm=marker
 */
