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

#ifdef ZEND_FIBER_VALGRIND
#include "valgrind/valgrind.h"
#endif

#include "php.h"
#include "zend.h"

#include "fiber_stack.h"

zend_bool zend_fiber_stack_allocate(zend_fiber_stack *stack, unsigned int size)
{
	static __thread size_t page_size;

	if (!page_size) {
		page_size = ZEND_FIBER_PAGESIZE;
	}

	size_t msize;

	stack->size = ((size_t) size + page_size - 1) / page_size * page_size;

#ifdef ZEND_FIBER_MMAP

	void *pointer;

	msize = stack->size + ZEND_FIBER_GUARDPAGES * page_size;
	pointer = mmap(0, msize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (pointer == (void *) -1) {
		pointer = mmap(0, msize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

		if (pointer == (void *) -1) {
			return 0;
		}
	}

#if ZEND_FIBER_GUARDPAGES
	mprotect(pointer, ZEND_FIBER_GUARDPAGES * page_size, PROT_NONE);
#endif

	stack->pointer = (void *)((char *) pointer + ZEND_FIBER_GUARDPAGES * page_size);
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
	stack->valgrind = VALGRIND_STACK_REGISTER(base, base + msize - ZEND_FIBER_GUARDPAGES * page_size);
#endif

	return 1;
}

void zend_fiber_stack_free(zend_fiber_stack *stack)
{
	static __thread size_t page_size;

	if (!page_size) {
		page_size = ZEND_FIBER_PAGESIZE;
	}

	if (stack->pointer != NULL) {
#ifdef VALGRIND_STACK_DEREGISTER
		VALGRIND_STACK_DEREGISTER(stack->valgrind);
#endif

#ifdef ZEND_FIBER_MMAP

		void *address;
		size_t len;

		address = (void *)((char *) stack->pointer - ZEND_FIBER_GUARDPAGES * page_size);
		len = stack->size + ZEND_FIBER_GUARDPAGES * page_size;

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
