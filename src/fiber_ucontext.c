/*
  +--------------------------------------------------------------------+
  | ext-fiber                                                          |
  +--------------------------------------------------------------------+
  | Redistribution and use in source and binary forms, with or without |
  | modification, are permitted provided that the conditions mentioned |
  | in the accompanying LICENSE file are met.                          |
  +--------------------------------------------------------------------+
  | Authors: Martin Schröder <m.schroeder2007@gmail.com>               |
  +--------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ucontext.h>

#include "php.h"
#include "zend.h"

#include "fiber.h"
#include "fiber_stack.h"

typedef struct _zend_fiber_context_ucontext zend_fiber_context_ucontext;

struct _zend_fiber_context_ucontext {
    ucontext_t ctx;
    zend_fiber_stack stack;
    zend_fiber_context_ucontext *caller;
    zend_bool initialized;
    zend_bool root;
};

char *zend_fiber_backend_info()
{
    return "ucontext (POSIX.1-2001, deprecated since POSIX.1-2004)";
}

zend_fiber_context zend_fiber_create_root_context()
{
    zend_fiber_context_ucontext *context;

    context = emalloc(sizeof(zend_fiber_context_ucontext));
    ZEND_SECURE_ZERO(context, sizeof(zend_fiber_context_ucontext));

    context->initialized = 1;
    context->root = 1;

    return (zend_fiber_context) context;
}

zend_fiber_context zend_fiber_create_context()
{
    zend_fiber_context_ucontext *context;

    context = emalloc(sizeof(zend_fiber_context_ucontext));
    ZEND_SECURE_ZERO(context, sizeof(zend_fiber_context_ucontext));

    return (zend_fiber_context) context;
}

zend_bool zend_fiber_create(zend_fiber_context ctx, zend_fiber_func func, size_t stack_size)
{
    zend_fiber_context_ucontext *context;

    context = (zend_fiber_context_ucontext *) ctx;

    if (UNEXPECTED(context->initialized == 1)) {
        return 0;
    }

    if (!zend_fiber_stack_allocate(&context->stack, stack_size)) {
        return 0;
    }

    if (getcontext(&context->ctx) == -1) {
        return 0;
    }

    context->ctx.uc_link = 0;
    context->ctx.uc_stack.ss_sp = context->stack.pointer;
    context->ctx.uc_stack.ss_size = context->stack.size;
    context->ctx.uc_stack.ss_flags = 0;

    makecontext(&context->ctx, func, 0);

    context->initialized = 1;

    return 1;
}

void zend_fiber_destroy(zend_fiber_context ctx)
{
    zend_fiber_context_ucontext *context;

    context = (zend_fiber_context_ucontext *) ctx;

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
    zend_fiber_context_ucontext *from;
    zend_fiber_context_ucontext *to;

    if (UNEXPECTED(current == NULL) || UNEXPECTED(next == NULL)) {
        return 0;
    }

    from = (zend_fiber_context_ucontext *) current;
    to = (zend_fiber_context_ucontext *) next;

    if (UNEXPECTED(from->initialized == 0) || UNEXPECTED(to->initialized == 0)) {
        return 0;
    }

    to->caller = from;

    if (swapcontext(&from->ctx, &to->ctx) == -1) {
        return 0;
    }

    return 1;
}

zend_bool zend_fiber_yield(zend_fiber_context current)
{
    zend_fiber_context_ucontext *fiber;

    if (UNEXPECTED(current == NULL)) {
        return 0;
    }

    fiber = (zend_fiber_context_ucontext *) current;

    if (UNEXPECTED(fiber->initialized == 0)) {
        return 0;
    }

    if (swapcontext(&fiber->ctx, &fiber->caller->ctx) == -1) {
        return 0;
    }

    return 1;
}

/*
 * vim: sw=4 ts=4
 * vim600: fdm=marker
 */
