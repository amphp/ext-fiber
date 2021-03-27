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

#ifndef FIBER_STACK_H
#define FIBER_STACK_H

typedef struct _zend_fiber_stack {
	void *pointer;
	size_t size;

#ifdef HAVE_VALGRIND
	int valgrind;
#endif
} zend_fiber_stack;

zend_bool zend_fiber_stack_allocate(zend_fiber_stack *stack, unsigned int size);
void zend_fiber_stack_free(zend_fiber_stack *stack);

#if _POSIX_MAPPED_FILES
#include <sys/mman.h>
#include <limits.h>
#define ZEND_FIBER_PAGESIZE sysconf(_SC_PAGESIZE)
#else
#define ZEND_FIBER_PAGESIZE 4096
#endif

#define ZEND_FIBER_GUARD_PAGES 1

#define ZEND_FIBER_DEFAULT_STACK_SIZE (ZEND_FIBER_PAGESIZE * (((sizeof(void *)) < 8) ? 512 : 2048))

#endif

/*
 * vim: sw=4 ts=4
 * vim600: fdm=marker
 */
