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

#ifndef PHP_FIBER_H
#define PHP_FIBER_H

#include "fiber.h"

extern zend_module_entry fiber_module_entry;
#define phpext_fiber_ptr &fiber_module_entry

#define PHP_FIBER_VERSION "0.1.0"

#if defined(ZTS) && defined(COMPILE_DL_FIBER)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

ZEND_BEGIN_MODULE_GLOBALS(fiber)
	/* Root fiber context (main thread). */
	zend_fiber_context root;

	/* Active fiber, NULL when in main thread. */
	zend_fiber *current_fiber;

	/* Default fiber C stack size. */
	zend_long stack_size;

	/* Error to be thrown into a fiber (will be populated by throw()). */
	zval *error;

ZEND_END_MODULE_GLOBALS(fiber)

extern ZEND_DECLARE_MODULE_GLOBALS(fiber)

#define FIBER_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(fiber, v)

#endif

/*
 * vim: sw=4 ts=4
 * vim600: fdm=marker
 */
