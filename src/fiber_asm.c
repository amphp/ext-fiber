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

const char *zend_fiber_backend_info(void)
{
	return "assembler (boost.context v1.76.0.beta1)";
}

ZEND_NORETURN static void zend_fiber_trampoline(transfer_t transfer)
{
	zend_fiber_context *context = transfer.data;

	context->caller = transfer.context;

	context->function(context);

	context->context = NULL;

	zend_fiber_suspend_context(context);

	abort();
}

PHP_FIBER_API zend_fiber_context *zend_fiber_create_context(zend_fiber_coroutine coroutine, size_t stack_size, void *data)
{
	zend_fiber_context *context = emalloc(sizeof(zend_fiber_context));
	ZEND_SECURE_ZERO(context, sizeof(zend_fiber_context));

	if (UNEXPECTED(!zend_fiber_stack_allocate(&context->stack, stack_size))) {
		efree(context);
		return NULL;
	}

	// Stack grows down, calculate the top of the stack. make_fcontext then shifts pointer to lower 16-byte boundary.
	void *stack = (void *) ((uintptr_t) context->stack.pointer + context->stack.size);

    context->context = make_fcontext(stack, context->stack.size, zend_fiber_trampoline);

	if (UNEXPECTED(!context->context)) {
		zend_fiber_stack_free(&context->stack);
		efree(context);
		return NULL;
	}

	context->function = coroutine;
	context->data = data;

	return context;
}

PHP_FIBER_API void zend_fiber_destroy_context(zend_fiber_context *context)
{
	if (context == NULL) {
		return;
	}

	zend_fiber_stack_free(&context->stack);

	efree(context);
}

PHP_FIBER_API zend_fiber_context *zend_fiber_switch_context(zend_fiber_context *to)
{
	ZEND_ASSERT(to && to->context && to->stack.pointer && "Invalid fiber context");

	transfer_t transfer = jump_fcontext(to->context, to);

	to->context = transfer.context;

	return transfer.data;
}

PHP_FIBER_API zend_fiber_context *zend_fiber_suspend_context(zend_fiber_context *current)
{
	ZEND_ASSERT(current && current->caller && current->stack.pointer && "Invalid fiber context");

	transfer_t transfer = jump_fcontext(current->caller, current);

	current->caller = transfer.context;

	return transfer.data;
}

/*
 * vim: sw=4 ts=4
 * vim600: fdm=marker
 */
