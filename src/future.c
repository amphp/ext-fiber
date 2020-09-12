/*
  +--------------------------------------------------------------------+
  | ext-fiber                                                          |
  +--------------------------------------------------------------------+
  | Redistribution and use in source and binary forms, with or without |
  | modification, are permitted provided that the conditions mentioned |
  | in the accompanying LICENSE file are met.                          |
  +--------------------------------------------------------------------+
  | Authors: Aaron Piotrowski <aaron@trowski.com>                      |
  +--------------------------------------------------------------------+
*/

#include "php.h"
#include "zend.h"

#include "future.h"

ZEND_API zend_class_entry *zend_ce_future;

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_future_schedule, 0, 1, IS_VOID, 0)
	ZEND_ARG_OBJ_INFO(0, fiber, Fiber, 0)
ZEND_END_ARG_INFO()

static const zend_function_entry future_methods[] = {
	ZEND_ABSTRACT_ME(Awaitable, schedule, arginfo_future_schedule)
	ZEND_FE_END
};

void zend_future_ce_register()
{
	zend_class_entry ce;
	
	INIT_CLASS_ENTRY(ce, "Future", future_methods);
	zend_ce_future = zend_register_internal_interface(&ce);
}
