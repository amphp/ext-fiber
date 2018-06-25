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
#include "fiber_stack.h"

typedef void* fcontext_t;

typedef struct _transfer_t {
    fcontext_t ctx;
    void *data;
} transfer_t;

extern fcontext_t make_fcontext(void *sp, size_t size, void (*fn)(transfer_t));
extern transfer_t jump_fcontext(fcontext_t to, void *vp);

typedef struct _zend_fiber_context_asm {
	fcontext_t ctx;
	fcontext_t caller;
	zend_fiber_stack stack;
	zend_bool initialized;
	zend_bool root;
} zend_fiber_context_asm;

typedef struct _zend_fiber_record_asm {
	zend_fiber_func func;
} zend_fiber_record_asm;

void zend_fiber_asm_start(transfer_t trans)
{
	zend_fiber_record_asm *record;
	zend_fiber_context_asm *context;

	record = (zend_fiber_record_asm *) trans.data;

	trans = jump_fcontext(trans.ctx, 0);
	context = (zend_fiber_context_asm *) trans.data;

	if (context != NULL) {
		context->caller = trans.ctx;
	}

	record->func();
}

zend_fiber_context zend_fiber_create_root_context()
{
	zend_fiber_context_asm *context;

	context = emalloc(sizeof(zend_fiber_context_asm));
	ZEND_SECURE_ZERO(context, sizeof(zend_fiber_context_asm));

	context->initialized = 1;
	context->root = 1;

	return (zend_fiber_context) context;
}

zend_fiber_context zend_fiber_create_context()
{
	zend_fiber_context_asm *context;

	context = emalloc(sizeof(zend_fiber_context_asm));
	ZEND_SECURE_ZERO(context, sizeof(zend_fiber_context_asm));

	return (zend_fiber_context) context;
}

zend_bool zend_fiber_create(zend_fiber_context ctx, zend_fiber_func func, size_t stack_size)
{
	static __thread size_t record_size;

	zend_fiber_context_asm *context;
	zend_fiber_record_asm *record;

	context = (zend_fiber_context_asm *) ctx;

	if (UNEXPECTED(context->initialized == 1)) {
		return 0;
	}

	if (!zend_fiber_stack_allocate(&context->stack, stack_size)) {
		return 0;
	}

	if (!record_size) {
		record_size = (size_t) ceil((double) sizeof(zend_fiber_record_asm) / 64) * 64;
	}

	void *sp = (void *) (context->stack.size - record_size + (char *) context->stack.pointer);

	record = (zend_fiber_record_asm *) sp;
	record->func = func;

	sp -= 64;

	context->ctx = make_fcontext(sp, sp - (void *) context->stack.pointer, &zend_fiber_asm_start);
	context->ctx = jump_fcontext(context->ctx, record).ctx;

	context->initialized = 1;

	return 1;
}

void zend_fiber_destroy(zend_fiber_context ctx)
{
	zend_fiber_context_asm *context;

	context = (zend_fiber_context_asm *) ctx;

	if (context != NULL) {
		if (!context->root && context->initialized) {
			zend_fiber_stack_free(&context->stack);
		}

		efree(context);
		context = NULL;
	}
}

zend_bool zend_fiber_switch_context(zend_fiber_context current, zend_fiber_context next)
{
	zend_fiber_context_asm *from;
	zend_fiber_context_asm *to;

	if (UNEXPECTED(current == NULL) || UNEXPECTED(next == NULL)) {
		return 0;
	}

	from = (zend_fiber_context_asm *) current;
	to = (zend_fiber_context_asm *) next;

	if (UNEXPECTED(from->initialized == 0) || UNEXPECTED(to->initialized == 0)) {
		return 0;
	}

	to->ctx = jump_fcontext(to->ctx, to).ctx;

	return 1;
}

zend_bool zend_fiber_yield(zend_fiber_context current)
{
	zend_fiber_context_asm *fiber;

	if (UNEXPECTED(current == NULL)) {
		return 0;
	}

	fiber = (zend_fiber_context_asm *) current;

	if (UNEXPECTED(fiber->initialized == 0)) {
		return 0;
	}

	fiber->caller = jump_fcontext(fiber->caller, 0).ctx;

	return 1;
}

/*
 * vim: sw=4 ts=4
 * vim600: fdm=marker
 */
