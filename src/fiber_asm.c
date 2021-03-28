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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "zend.h"

#include "fiber.h"

extern fcontext_t make_fcontext(void *sp, size_t size, void (*fn)(transfer_t));
extern transfer_t jump_fcontext(fcontext_t to, void *vp);

typedef struct _zend_fiber_record {
	zend_fiber_func func;
} zend_fiber_record;

char *zend_fiber_backend_info(void)
{
	return "assembler (boost.context v1.76.0.beta1)";
}

void zend_fiber_asm_start(transfer_t trans)
{
	zend_fiber_record *record = (zend_fiber_record *) trans.data;
	zend_fiber_context *context;

	trans = jump_fcontext(trans.ctx, 0);
	context = (zend_fiber_context *) trans.data;

	if (context != NULL) {
		context->caller = trans.ctx;
	}

	record->func();
}

zend_fiber_context *zend_fiber_create_root_context(void)
{
	zend_fiber_context *context = emalloc(sizeof(zend_fiber_context));
	ZEND_SECURE_ZERO(context, sizeof(zend_fiber_context));

	context->initialized = 1;
	context->root = 1;

	return context;
}

zend_fiber_context *zend_fiber_create_context(void)
{
	zend_fiber_context *context = emalloc(sizeof(zend_fiber_context));
	ZEND_SECURE_ZERO(context, sizeof(zend_fiber_context));

	return context;
}

zend_bool zend_fiber_create(zend_fiber_context *context, zend_fiber_func func, size_t stack_size)
{
	if (UNEXPECTED(context->initialized == 1)) {
		return 0;
	}

	if (!zend_fiber_stack_allocate(&context->stack, stack_size)) {
		return 0;
	}

	// Ensure 16-byte alignment for stack pointer.
	zend_fiber_record *record = (zend_fiber_record *) ((context->stack.size - sizeof(zend_fiber_record) + (uintptr_t) context->stack.pointer) & (uintptr_t) ~0xf);
	record->func = func;

	// 64-byte gap between zend_fiber_record structure and top.
	void *stack_top = (void *) ((uintptr_t) record - (uintptr_t) 64);
	void *stack_bottom = (void *) ((uintptr_t) stack_top - (uintptr_t) context->stack.pointer);
	const size_t size = (uintptr_t) stack_top - (uintptr_t) stack_bottom;

	context->ctx = make_fcontext(stack_top, size, &zend_fiber_asm_start);

	ZEND_ASSERT(context->ctx && "Could not create new fiber context");

	context->ctx = jump_fcontext(context->ctx, record).ctx;

	context->initialized = 1;

	return 1;
}

void zend_fiber_destroy(zend_fiber_context *context)
{
	if (context != NULL) {
		if (!context->root && context->initialized) {
			zend_fiber_stack_free(&context->stack);
		}

		efree(context);
		context = NULL;
	}
}

zend_bool zend_fiber_switch_context(zend_fiber_context *current, zend_fiber_context *next)
{
	if (UNEXPECTED(current == NULL) || UNEXPECTED(next == NULL)) {
		return 0;
	}

	zend_fiber_context *from = current;
	zend_fiber_context *to = next;

	if (UNEXPECTED(from->initialized == 0) || UNEXPECTED(to->initialized == 0)) {
		return 0;
	}

	to->ctx = jump_fcontext(to->ctx, to).ctx;

	return 1;
}

zend_bool zend_fiber_suspend_context(zend_fiber_context *current)
{
	if (UNEXPECTED(current == NULL)) {
		return 0;
	}

	if (UNEXPECTED(current->initialized == 0)) {
		return 0;
	}

	current->caller = jump_fcontext(current->caller, 0).ctx;

	return 1;
}

/*
 * vim: sw=4 ts=4
 * vim600: fdm=marker
 */
