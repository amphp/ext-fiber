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
# include "config.h"
#endif

#include "fiber.h"

typedef void *fcontext_t;

typedef struct _transfer_t {
	fcontext_t context;
	void *data;
} transfer_t;

extern fcontext_t make_fcontext(void *sp, size_t size, void (*fn)(transfer_t));
extern transfer_t jump_fcontext(fcontext_t to, void *vp);

const char *zend_fiber_backend_info(void)
{
	return "assembler (boost.context v1.76.0)";
}

static ZEND_NORETURN void zend_fiber_trampoline(transfer_t transfer)
{
	zend_fiber_context *context = transfer.data;

	context->caller = transfer.context;

	context->function(context);

	context->self = NULL;

	zend_fiber_suspend_context(context);

	abort();
}

PHP_FIBER_API zend_bool zend_fiber_init_context(zend_fiber_context *context, zend_fiber_coroutine coroutine, size_t stack_size)
{
	if (UNEXPECTED(!zend_fiber_stack_allocate(&context->stack, stack_size))) {
		return 0;
	}

	// Stack grows down, calculate the top of the stack. make_fcontext then shifts pointer to lower 16-byte boundary.
	void *stack = (void *) ((uintptr_t) context->stack.pointer + context->stack.size);

	context->self = make_fcontext(stack, context->stack.size, zend_fiber_trampoline);

	if (UNEXPECTED(!context->self)) {
		zend_fiber_stack_free(&context->stack);
		return 0;
	}

	context->function = coroutine;
	context->caller = NULL;

	return 1;
}

PHP_FIBER_API void zend_fiber_destroy_context(zend_fiber_context *context)
{
	zend_fiber_stack_free(&context->stack);
}

PHP_FIBER_API void zend_fiber_switch_context(zend_fiber_context *to)
{
	ZEND_ASSERT(to && to->self && to->stack.pointer && "Invalid fiber context");

	transfer_t transfer = jump_fcontext(to->self, to);

	to->self = transfer.context;
}

PHP_FIBER_API void zend_fiber_suspend_context(zend_fiber_context *current)
{
	ZEND_ASSERT(current && current->caller && current->stack.pointer && "Invalid fiber context");

	transfer_t transfer = jump_fcontext(current->caller, NULL);

	current->caller = transfer.context;
}

/*
 * vim: sw=4 ts=4
 * vim600: fdm=marker
 */
