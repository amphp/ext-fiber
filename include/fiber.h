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

extern ZEND_API zend_class_entry *zend_ce_awaitable;
extern ZEND_API zend_class_entry *zend_ce_fiber_scheduler;

typedef void* zend_fiber_context;

typedef struct _zend_fiber_continuation zend_fiber_continuation;

struct _zend_fiber_continuation {
	/* Continuation closure provided to Awaitable::onResolve(). */
	zval closure;

	/* Value to return from suspend when resuming the fiber (will be populated by resume()). */
	zval value;

	/* Error to be thrown into a fiber (will be populated by throw()). */
	zval *error;
};

typedef struct _zend_fiber zend_fiber;

struct _zend_fiber {
	/* Fiber PHP object handle. */
	zend_object std;

	/* Status of the fiber, one of the ZEND_FIBER_STATUS_* constants. */
	zend_uchar status;
	
	/* Used to determine which fiber entered a scheduler and which scheduler should resume a fiber. */
	zend_fiber *link;

	/* Continuation variables for non-scheduler fibers. */
	zend_fiber_continuation *continuation;

	/* Callback and info / cache to be used when fiber is started. */
	zend_fcall_info fci;
	zend_fcall_info_cache fci_cache;

	/* Fiber context of this fiber, will be created during call to start(). */
	zend_fiber_context context;

	/* Current Zend VM execute data being run by the fiber. */
	zend_execute_data *exec;

	/* VM stack being used by the fiber. */
	zend_vm_stack stack;

	/* Max size of the C stack being used by the fiber. */
	size_t stack_size;
};

static const zend_uchar ZEND_FIBER_STATUS_INIT = 0;
static const zend_uchar ZEND_FIBER_STATUS_SUSPENDED = 0x1;
static const zend_uchar ZEND_FIBER_STATUS_RUNNING = 0x2;
static const zend_uchar ZEND_FIBER_STATUS_RETURNED = 0x4;
static const zend_uchar ZEND_FIBER_STATUS_THREW = 0x8;
static const zend_uchar ZEND_FIBER_STATUS_SHUTDOWN = 0x10;

static const zend_uchar ZEND_FIBER_STATUS_FINISHED = 0xc;

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

END_EXTERN_C()

#define ZEND_FIBER_VM_STACK_SIZE 4096

#endif

/*
 * vim: sw=4 ts=4
 * vim600: fdm=marker
 */
