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

#ifdef HAVE_VALGRIND
#include "valgrind/valgrind.h"
#endif

/*
 * FreeBSD require a first (i.e. addr) argument of mmap(2) is not NULL
 * if MAP_STACK is passed.
 * http://www.FreeBSD.org/cgi/query-pr.cgi?pr=158755
 */
#if defined(MAP_STACK) && !defined(__FreeBSD__) && !defined(__FreeBSD_kernel__)
# define ZEND_FIBER_STACK_FLAGS (MAP_PRIVATE | MAP_ANON | MAP_STACK)
#else
# define ZEND_FIBER_STACK_FLAGS (MAP_PRIVATE | MAP_ANON)
#endif

zend_bool zend_fiber_stack_allocate(zend_fiber_stack *stack, size_t size)
{
	void *pointer;

	stack->size = (size + ZEND_FIBER_PAGESIZE - 1) / ZEND_FIBER_PAGESIZE * ZEND_FIBER_PAGESIZE;
	size_t msize = stack->size + ZEND_FIBER_GUARD_PAGES * ZEND_FIBER_PAGESIZE;

#ifdef PHP_WIN32
	pointer = VirtualAlloc(0, msize, MEM_COMMIT, PAGE_READWRITE);

	if (!pointer) {
		return 0;
	}

# if ZEND_FIBER_GUARD_PAGES
	DWORD protect;

	if (!VirtualProtect(pointer, ZEND_FIBER_GUARD_PAGES * ZEND_FIBER_PAGESIZE, PAGE_READWRITE | PAGE_GUARD, &protect)) {
		VirtualFree(pointer, 0, MEM_RELEASE);
		return 0;
	}
# endif
#else
	pointer = mmap(NULL, msize, PROT_READ | PROT_WRITE, ZEND_FIBER_STACK_FLAGS, -1, 0);

	if (pointer == MAP_FAILED) {
		return 0;
	}

# if ZEND_FIBER_GUARD_PAGES
	if (mprotect(pointer, ZEND_FIBER_GUARD_PAGES * ZEND_FIBER_PAGESIZE, PROT_NONE) < 0) {
		munmap(pointer, msize);
		return 0;
	}
# endif
#endif

	stack->pointer = (void *) ((char *) pointer + ZEND_FIBER_GUARD_PAGES * ZEND_FIBER_PAGESIZE);

#ifdef VALGRIND_STACK_REGISTER
	char *base = (char *) stack->pointer;
	stack->valgrind = VALGRIND_STACK_REGISTER(base, base + msize - ZEND_FIBER_GUARD_PAGES * ZEND_FIBER_PAGESIZE);
#endif

	return 1;
}

void zend_fiber_stack_free(zend_fiber_stack *stack)
{
	if (stack->pointer == NULL) {
		return;
	}

#ifdef VALGRIND_STACK_DEREGISTER
	VALGRIND_STACK_DEREGISTER(stack->valgrind);
#endif

	void *pointer = (void *) ((char *) stack->pointer - ZEND_FIBER_GUARD_PAGES * ZEND_FIBER_PAGESIZE);

#ifdef PHP_WIN32
	VirtualFree(pointer, 0, MEM_RELEASE);
#else
	size_t length = stack->size + ZEND_FIBER_GUARD_PAGES * ZEND_FIBER_PAGESIZE;

	munmap(pointer, length);
#endif

	stack->pointer = NULL;
}

/*
 * vim: sw=4 ts=4
 * vim600: fdm=marker
 */
