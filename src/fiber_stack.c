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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "zend.h"
#include "fiber_stack.h"

#ifdef HAVE_VALGRIND
#include "valgrind/valgrind.h"
#endif

zend_bool zend_fiber_stack_allocate(zend_fiber_stack *stack, unsigned int size)
{
	size_t msize;

	stack->size = ((size_t) size + ZEND_FIBER_PAGESIZE - 1) / ZEND_FIBER_PAGESIZE * ZEND_FIBER_PAGESIZE;

#ifdef ZEND_FIBER_MMAP

	void *pointer;
	int mapflags = MAP_PRIVATE | MAP_ANONYMOUS;
#ifdef __OpenBSD__
	mapflags |= MAP_STACK;
#endif

	msize = stack->size + ZEND_FIBER_GUARD_PAGES * ZEND_FIBER_PAGESIZE;
	pointer = mmap(0, msize, PROT_READ | PROT_WRITE, mapflags, -1, 0);

	if (pointer == (void *) -1) {
		return 0;
	}

#if ZEND_FIBER_GUARD_PAGES
	mprotect(pointer, ZEND_FIBER_GUARD_PAGES * ZEND_FIBER_PAGESIZE, PROT_NONE);
#endif

	stack->pointer = (void *)((char *) pointer + ZEND_FIBER_GUARD_PAGES * ZEND_FIBER_PAGESIZE);
#else
	stack->pointer = emalloc_large(stack->size);
	msize = stack->size;
#endif

	if (!stack->pointer) {
		return 0;
	}

#ifdef VALGRIND_STACK_REGISTER
	char * base;

	base = (char *) stack->pointer;
	stack->valgrind = VALGRIND_STACK_REGISTER(base, base + msize - ZEND_FIBER_GUARD_PAGES * ZEND_FIBER_PAGESIZE);
#endif

	return 1;
}

void zend_fiber_stack_free(zend_fiber_stack *stack)
{
	if (stack->pointer != NULL) {
#ifdef VALGRIND_STACK_DEREGISTER
		VALGRIND_STACK_DEREGISTER(stack->valgrind);
#endif

#ifdef ZEND_FIBER_MMAP

		void *address;
		size_t len;

		address = (void *)((char *) stack->pointer - ZEND_FIBER_GUARD_PAGES * ZEND_FIBER_PAGESIZE);
		len = stack->size + ZEND_FIBER_GUARD_PAGES * ZEND_FIBER_PAGESIZE;

		munmap(address, len);
#else
		efree(stack->pointer);
#endif

		stack->pointer = NULL;
	}
}

/*
 * vim: sw=4 ts=4
 * vim600: fdm=marker
 */
