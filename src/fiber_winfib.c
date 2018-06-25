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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "zend.h"

#include "fiber.h"

typedef struct _zend_fiber_context_win32 {
	void *fiber;
	void *caller;
	zend_bool root;
	zend_bool initialized;
} zend_fiber_context_win32;

zend_fiber_context zend_fiber_create_root_context()
{
	zend_fiber_context_win32 *context;

	context = (zend_fiber_context_win32 *) zend_fiber_create_context();
	context->root = 1;
	context->initialized = 1;

	if (IsThreadAFiber()) {
		context->fiber = GetCurrentFiber();
	} else {
		context->fiber = ConvertThreadToFiberEx(0, FIBER_FLAG_FLOAT_SWITCH);
	}

	if (context->fiber == NULL) {
		return NULL;
	}

	return (zend_fiber_context)context;
}

zend_fiber_context zend_fiber_create_context()
{
	zend_fiber_context_win32 *context;

	context = emalloc(sizeof(zend_fiber_context_win32));
	ZEND_SECURE_ZERO(context, sizeof(zend_fiber_context_win32));

	return (zend_fiber_context) context;
}

zend_bool zend_fiber_create(zend_fiber_context ctx, zend_fiber_func func, size_t stack_size)
{
	zend_fiber_context_win32 *context;

	context = (zend_fiber_context_win32 *) ctx;

	if (UNEXPECTED(context->initialized == 1)) {
		return 0;
	}

	context->fiber = CreateFiberEx(stack_size, stack_size, FIBER_FLAG_FLOAT_SWITCH, (void (*)(void *))func, context);

	if (context->fiber == NULL) {
		return 0;
	}

	context->initialized = 1;

	return 1;
}

void zend_fiber_destroy(zend_fiber_context ctx)
{
	zend_fiber_context_win32 *context;

	context = (zend_fiber_context_win32 *) ctx;

	if (context != NULL) {
		if (context->root) {
			ConvertFiberToThread();
		} else if (context->initialized) {
			DeleteFiber(context->fiber);
		}

		efree(context);
		context = NULL;
	}
}

zend_bool zend_fiber_switch_context(zend_fiber_context current, zend_fiber_context next)
{
	zend_fiber_context_win32 *from;
	zend_fiber_context_win32 *to;

	if (UNEXPECTED(current == NULL) || UNEXPECTED(next == NULL)) {
		return 0;
	}

	from = (zend_fiber_context_win32 *) current;
	to = (zend_fiber_context_win32 *) next;

	if (UNEXPECTED(from->initialized == 0) || UNEXPECTED(to->initialized == 0)) {
		return 0;
	}

	to->caller = from->fiber;
	SwitchToFiber(to->fiber);
	to->caller = NULL;

	return 1;
}

zend_bool zend_fiber_yield(zend_fiber_context current)
{
	zend_fiber_context_win32 *from;

	if (UNEXPECTED(current == NULL)) {
		return 0;
	}

	from = (zend_fiber_context_win32 *) current;

	if (UNEXPECTED(from->initialized == 0)) {
		return 0;
	}

	SwitchToFiber(from->caller);

	return 1;
}

/*
 * vim: sw=4 ts=4
 * vim600: fdm=marker
 */
