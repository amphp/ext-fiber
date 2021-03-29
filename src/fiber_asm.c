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

char *zend_fiber_backend_info(void)
{
	return "assembler (boost.context v1.76.0.beta1)";
}

static void zend_fiber_initialize(transfer_t trans)
{
	zend_fiber_context *context = (zend_fiber_context *) trans.data;

	ZEND_ASSERT(context && "Fiber initialization failure!");

	context->caller = trans.ctx;

	context->function();
}

zend_fiber_context *zend_fiber_create_context(zend_fiber_function function, size_t stack_size)
{
	zend_fiber_context *context = emalloc(sizeof(zend_fiber_context));
	ZEND_SECURE_ZERO(context, sizeof(zend_fiber_context));

	if (UNEXPECTED(!zend_fiber_stack_allocate(&context->stack, stack_size))) {
		return NULL;
	}

	// Ensure 16-byte alignment for stack pointer.
	void *stack = (void *) (((uintptr_t) context->stack.pointer + (uintptr_t) context->stack.size) & (uintptr_t) ~0xf);
	const size_t size = (uintptr_t) stack - (uintptr_t) context->stack.pointer;

	context->function = function;

	context->ctx = make_fcontext(stack, size, &zend_fiber_initialize);

	if (UNEXPECTED(!context->ctx)) {
		zend_fiber_stack_free(&context->stack);
		return NULL;
	}

	return context;
}

void zend_fiber_destroy_context(zend_fiber_context *context)
{
	if (context == NULL) {
		return;
	}

	if (context->stack.pointer != NULL) {
		zend_fiber_stack_free(&context->stack);
	}

	efree(context);
}

zend_bool zend_fiber_switch_context(zend_fiber_context *to)
{
	if (UNEXPECTED(to == NULL)) {
		return 0;
	}

	if (UNEXPECTED(to->stack.pointer == NULL)) {
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

	if (UNEXPECTED(current->stack.pointer == NULL)) {
		return 0;
	}

	current->caller = jump_fcontext(current->caller, NULL).ctx;

	return 1;
}

/*
 * vim: sw=4 ts=4
 * vim600: fdm=marker
 */
