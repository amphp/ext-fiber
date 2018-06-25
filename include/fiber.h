/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2018 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Martin Schröder <m.schroeder2007@gmail.com>                 |
  +----------------------------------------------------------------------+
*/

#ifndef FIBER_H
#define FIBER_H

#include "php.h"

BEGIN_EXTERN_C()

void zend_fiber_ce_register();
void zend_fiber_ce_unregister();

void zend_fiber_shutdown();

typedef void* zend_fiber_context;
typedef struct _zend_fiber zend_fiber;

struct _zend_fiber {
	/* Fiber PHP object handle. */
	zend_object std;

	/* Status of the fiber, one of the ZEND_FIBER_STATUS_* constants. */
	zend_uchar status;

	/* Callback and info / cache to be used when fiber is started. */
	zend_fcall_info fci;
	zend_fcall_info_cache fci_cache;

	/* Fiber context of this fiber, will be created during call to start(). */
	zend_fiber_context context;

	/* Destination for a PHP value being passed into or returned from the fiber. */
	zval *value;

	/* Current Zend VM execute data being run by the fiber. */
	zend_execute_data *exec;

	/* VM stack being used by the fiber. */
	zend_vm_stack stack;

	/* Max size of the C stack being used by the fiber. */
	size_t stack_size;
};

static const zend_uchar ZEND_FIBER_STATUS_INIT = 0;
static const zend_uchar ZEND_FIBER_STATUS_SUSPENDED = 1;
static const zend_uchar ZEND_FIBER_STATUS_RUNNING = 2;
static const zend_uchar ZEND_FIBER_STATUS_FINISHED = 3;
static const zend_uchar ZEND_FIBER_STATUS_DEAD = 4;

typedef void (* zend_fiber_func)();

zend_fiber_context zend_fiber_create_root_context();
zend_fiber_context zend_fiber_create_context();

zend_bool zend_fiber_create(zend_fiber_context context, zend_fiber_func func, size_t stack_size);
void zend_fiber_destroy(zend_fiber_context context);

zend_bool zend_fiber_switch_context(zend_fiber_context current, zend_fiber_context next);
zend_bool zend_fiber_yield(zend_fiber_context current);

END_EXTERN_C()

#define REGISTER_FIBER_CLASS_CONST_LONG(const_name, value) \
	zend_declare_class_constant_long(zend_ce_fiber, const_name, sizeof(const_name)-1, (zend_long)value);

#define ZEND_FIBER_VM_STACK_SIZE 4096

#endif

/*
 * vim: sw=4 ts=4
 * vim600: fdm=marker
 */
