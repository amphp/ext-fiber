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
	/* Active fiber, NULL when in main thread. */
	zend_fiber *current_fiber;

	/* Next fiber ID. */
	zend_long id;

	/* Default fiber C stack size. */
	zend_long stack_size;

	/* Currently executing fibers. */
	HashTable fibers;

	/* Flag if a shutdown function has been invoked. */
	zend_bool shutdown;

	/* ZEND_CATCH handler that may be declared by another extension. */
	user_opcode_handler_t catch_handler;

ZEND_END_MODULE_GLOBALS(fiber)

extern ZEND_DECLARE_MODULE_GLOBALS(fiber)

#define FIBER_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(fiber, v)

#endif

/*
 * vim: sw=4 ts=4
 * vim600: fdm=marker
 */
