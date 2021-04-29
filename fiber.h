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

#ifndef FIBER_H
#define FIBER_H

#include "php.h"

BEGIN_EXTERN_C()

void zend_register_fiber_ce(void);

void zend_fiber_init(void);
void zend_fiber_shutdown(void);

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

typedef struct _zend_fiber_context zend_fiber_context;

typedef void (*zend_fiber_coroutine)(zend_fiber_context *context);

typedef struct _zend_fiber_stack {
	void *pointer;
	size_t size;

#ifdef HAVE_VALGRIND
	int valgrind;
#endif
} zend_fiber_stack;

typedef struct _zend_fiber_context {
	void *self;
	void *caller;
	zend_fiber_coroutine function;
	zend_fiber_stack stack;
} zend_fiber_context;

#define ZEND_FIBER_DEFAULT_STACK_SIZE (ZEND_FIBER_PAGESIZE * (((sizeof(void *)) < 8) ? 512 : 2048))

typedef struct _zend_fiber {
	/* Fiber PHP object handle. */
	zend_object std;

	/* Status of the fiber, one of the ZEND_FIBER_STATUS_* constants. */
	zend_uchar status;

	/* Callback and info / cache to be used when fiber is started. */
	zend_fcall_info fci;
	zend_fcall_info_cache fci_cache;

	/* Context of this fiber, will be initialized during call to Fiber::start(). */
	zend_fiber_context context;

	/* Current Zend VM execute data being run by the fiber. */
	zend_execute_data *execute_data;

	/* Exception to be thrown from Fiber::suspend(). */
	zval *exception;

	/* Storage for temporaries and fiber return value. */
	zval value;
} zend_fiber;

typedef struct _zend_fiber_reflection {
	/* ReflectionFiber PHP object handle. */
	zend_object std;

	/* Fiber being reflected. */
	zend_fiber *fiber;
} zend_fiber_reflection;

typedef struct _zend_fiber_error {
	int type;
	const char *filename;
	uint32_t lineno;
	zend_string *message;
} zend_fiber_error;

PHP_FIBER_API zend_fiber *zend_get_current_fiber(void);
PHP_FIBER_API zend_bool zend_is_fiber_exit(const zend_object *exception);

typedef void (*zend_observer_fiber_switch_handler)(zend_fiber *from, zend_fiber *to);

PHP_FIBER_API void zend_observer_fiber_switch_register(zend_observer_fiber_switch_handler handler);

static const zend_uchar ZEND_FIBER_STATUS_INIT = 0;
static const zend_uchar ZEND_FIBER_STATUS_SUSPENDED = 0x1;
static const zend_uchar ZEND_FIBER_STATUS_RUNNING = 0x2;
static const zend_uchar ZEND_FIBER_STATUS_RETURNED = 0x4;
static const zend_uchar ZEND_FIBER_STATUS_THREW = 0x8;
static const zend_uchar ZEND_FIBER_STATUS_SHUTDOWN = 0x10;

static const zend_uchar ZEND_FIBER_STATUS_FINISHED = 0x1c;

const char *zend_fiber_backend_info(void);

PHP_FIBER_API zend_bool zend_fiber_init_context(zend_fiber_context *context, zend_fiber_coroutine coroutine, size_t stack_size);
PHP_FIBER_API void zend_fiber_destroy_context(zend_fiber_context *context);

zend_bool zend_fiber_stack_allocate(zend_fiber_stack *stack, size_t size);
void zend_fiber_stack_free(zend_fiber_stack *stack);

PHP_FIBER_API void zend_fiber_switch_context(zend_fiber_context *to);
PHP_FIBER_API void zend_fiber_suspend_context(zend_fiber_context *current);

void zend_fiber_error_observer(int type, const char *filename, uint32_t line, zend_string *message);

#define ZEND_FIBER_GUARD_PAGES 1

#define ZEND_FIBER_DEFAULT_C_STACK_SIZE (4096 * (((sizeof(void *)) < 8) ? 256 : 512))

#define ZEND_FIBER_VM_STACK_SIZE (1024 * sizeof(zval))

END_EXTERN_C()

#endif

/*
 * vim: sw=4 ts=4
 * vim600: fdm=marker
 */
