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

#ifdef HAVE_VALGRIND
# include <valgrind/valgrind.h>
#endif

#ifndef PHP_WIN32
# include <unistd.h>
# include <sys/mman.h>
# include <limits.h>
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

static size_t zend_fiber_page_size()
{
#if _POSIX_MAPPED_FILES
	static size_t page_size;

	if (!page_size) {
		page_size = sysconf(_SC_PAGESIZE);
	}

	return page_size;
#else
	return ZEND_FIBER_DEFAULT_PAGE_SIZE;
#endif
}

bool zend_fiber_stack_allocate(zend_fiber_stack *stack, size_t size)
{
	void *pointer;
	const size_t page_size = zend_fiber_page_size();

	ZEND_ASSERT(size >= page_size + ZEND_FIBER_GUARD_PAGES * page_size);

	stack->size = (size + page_size - 1) / page_size * page_size;
	const size_t msize = stack->size + ZEND_FIBER_GUARD_PAGES * page_size;

#ifdef PHP_WIN32
	pointer = VirtualAlloc(0, msize, MEM_COMMIT, PAGE_READWRITE);

	if (!pointer) {
		return false;
	}

# if ZEND_FIBER_GUARD_PAGES
	DWORD protect;

	if (!VirtualProtect(pointer, ZEND_FIBER_GUARD_PAGES * page_size, PAGE_READWRITE | PAGE_GUARD, &protect)) {
		VirtualFree(pointer, 0, MEM_RELEASE);
		return false;
	}
# endif
#else
	pointer = mmap(NULL, msize, PROT_READ | PROT_WRITE, ZEND_FIBER_STACK_FLAGS, -1, 0);

	if (pointer == MAP_FAILED) {
		return false;
	}

# if ZEND_FIBER_GUARD_PAGES
	if (mprotect(pointer, ZEND_FIBER_GUARD_PAGES * page_size, PROT_NONE) < 0) {
		munmap(pointer, msize);
		return false;
	}
# endif
#endif

	stack->pointer = (void *) ((uintptr_t) pointer + ZEND_FIBER_GUARD_PAGES * page_size);

#ifdef VALGRIND_STACK_REGISTER
	uintptr_t base = (uintptr_t) stack->pointer;
	stack->valgrind = VALGRIND_STACK_REGISTER(base, base + stack->size);
#endif

	return true;
}

void zend_fiber_stack_free(zend_fiber_stack *stack)
{
	if (!stack->pointer) {
		return;
	}

#ifdef VALGRIND_STACK_DEREGISTER
	VALGRIND_STACK_DEREGISTER(stack->valgrind);
#endif

	const size_t page_size = zend_fiber_page_size();

	void *pointer = (void *) ((uintptr_t) stack->pointer - ZEND_FIBER_GUARD_PAGES * page_size);

#ifdef PHP_WIN32
	VirtualFree(pointer, 0, MEM_RELEASE);
#else
	munmap(pointer, stack->size + ZEND_FIBER_GUARD_PAGES * page_size);
#endif

	stack->pointer = NULL;
}

/*
 * vim: sw=4 ts=4
 * vim600: fdm=marker
 */
