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
#include "zend_observer.h"

BEGIN_EXTERN_C()

void zend_fiber_ce_register(void);
void zend_fiber_ce_unregister(void);

void zend_fiber_startup(void);
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

typedef void (* zend_fiber_function)(void);

typedef void* fcontext_t;

typedef struct _transfer_t {
	fcontext_t ctx;
	void *data;
} transfer_t;

typedef struct _zend_fiber_stack {
	void *pointer;
	size_t size;

#ifdef HAVE_VALGRIND
	int valgrind;
#endif
} zend_fiber_stack;

typedef struct _zend_fiber_context {
	fcontext_t ctx;
	fcontext_t caller;
	zend_fiber_function function;
	zend_fiber_stack stack;
} zend_fiber_context;

zend_bool zend_fiber_stack_allocate(zend_fiber_stack *stack, unsigned int size);
void zend_fiber_stack_free(zend_fiber_stack *stack);

#if _POSIX_MAPPED_FILES
# include <sys/mman.h>
# include <limits.h>
# define ZEND_FIBER_PAGESIZE sysconf(_SC_PAGESIZE)
#else
# define ZEND_FIBER_PAGESIZE 4096
#endif

#define ZEND_FIBER_GUARD_PAGES 1

#define ZEND_FIBER_DEFAULT_STACK_SIZE (ZEND_FIBER_PAGESIZE * (((sizeof(void *)) < 8) ? 512 : 2048))

typedef struct _zend_fiber zend_fiber;

struct _zend_fiber {
	/* Fiber PHP object handle. */
	zend_object std;

	/* Unique ID assigned to this fiber. */
	zend_long id;

	/* Status of the fiber, one of the ZEND_FIBER_STATUS_* constants. */
	zend_uchar status;

	/* Callback and info / cache to be used when fiber is started. */
	zend_fcall_info fci;
	zend_fcall_info_cache fci_cache;

	/* Fiber context of this fiber, will be created during call to start(). */
	zend_fiber_context *context;

	/* Current Zend VM execute data being run by the fiber. */
	zend_execute_data *execute_data;

	/* Exception to be thrown from Fiber::suspend(). */
	zval *error;

	/* Storage for temporaries and fiber return value. */
	zval value;
};

typedef struct _zend_fiber_reflection zend_fiber_reflection;

struct _zend_fiber_reflection {
	/* ReflectionFiber PHP object handle. */
	zend_object std;

	/* Fiber being reflected. */
	zend_fiber *fiber;
};

PHP_FIBER_API zend_fiber *zend_get_current_fiber(void);
PHP_FIBER_API zend_bool zend_is_fiber_exit(zend_object *exception);

typedef void (*zend_observer_fiber_switch_handler)(zend_fiber *from, zend_fiber *to);

PHP_FIBER_API void zend_observer_fiber_switch_register(zend_observer_fiber_switch_handler handler);

static const zend_uchar ZEND_FIBER_STATUS_INIT = 0;
static const zend_uchar ZEND_FIBER_STATUS_SUSPENDED = 0x1;
static const zend_uchar ZEND_FIBER_STATUS_RUNNING = 0x2;
static const zend_uchar ZEND_FIBER_STATUS_RETURNED = 0x4;
static const zend_uchar ZEND_FIBER_STATUS_THREW = 0x8;
static const zend_uchar ZEND_FIBER_STATUS_SHUTDOWN = 0x10;

static const zend_uchar ZEND_FIBER_STATUS_FINISHED = 0x1c;

char *zend_fiber_backend_info(void);

zend_fiber_context *zend_fiber_create_root_context(void);
zend_fiber_context *zend_fiber_create_context(void);

zend_bool zend_fiber_create(zend_fiber_context *context, zend_fiber_function function, size_t stack_size);
void zend_fiber_destroy(zend_fiber_context *context);

zend_bool zend_fiber_switch_context(zend_fiber_context *to);
zend_bool zend_fiber_suspend_context(zend_fiber_context *current);

void zend_fiber_error_observer(int type, const char *filename, uint32_t line, zend_string *message);

END_EXTERN_C()

#define ZEND_FIBER_VM_STACK_SIZE (1024 * sizeof(zval))

#endif

/*
 * vim: sw=4 ts=4
 * vim600: fdm=marker
 */
