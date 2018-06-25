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

#ifndef PHP_TASK_H
#define PHP_TASK_H

#include "fiber.h"

extern zend_module_entry task_module_entry;

#define PHP_TASK_VERSION "0.1.0"

#if defined(ZTS) && defined(COMPILE_DL_TASK)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

ZEND_BEGIN_MODULE_GLOBALS(task)
	/* Root fiber context (main thread). */
	zend_fiber_context root;

	/* Active fiber, NULL when in main thread. */
	zend_fiber *current_fiber;

	/* Default fiber C stack size. */
	zend_long stack_size;

	/* Error to be thrown into a fiber (will be populated by throw()). */
	zval *error;

ZEND_END_MODULE_GLOBALS(task)

#define TASK_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(task, v)

#endif

/*
 * vim: sw=4 ts=4
 * vim600: fdm=marker
 */
