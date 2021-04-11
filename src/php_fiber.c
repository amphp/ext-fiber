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
#include "zend_observer.h"
#include "ext/standard/info.h"

#include "php_fiber.h"
#include "fiber.h"

ZEND_DECLARE_MODULE_GLOBALS(fiber)

static PHP_INI_MH(OnUpdateFiberStackSize)
{
	zend_long tmp;
	
	if (OnUpdateLongGEZero(entry, new_value, mh_arg1, mh_arg2, mh_arg3, stage) == FAILURE) {
		return FAILURE;
	}
	
	if (FIBER_G(stack_size) == 0) {
		FIBER_G(stack_size) = ZEND_FIBER_DEFAULT_STACK_SIZE;
		return SUCCESS;
	}

	FIBER_G(stack_size) += ZEND_FIBER_GUARD_PAGES;
	
	tmp = ZEND_FIBER_PAGESIZE * FIBER_G(stack_size);
	
	if (tmp / ZEND_FIBER_PAGESIZE != FIBER_G(stack_size)) {
		FIBER_G(stack_size) = ZEND_FIBER_DEFAULT_STACK_SIZE;
		return FAILURE;
	}
	
	FIBER_G(stack_size) = tmp;
	
	return SUCCESS;
}

PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("fiber.stack_size", "0", PHP_INI_ALL, OnUpdateFiberStackSize, stack_size, zend_fiber_globals, fiber_globals)
PHP_INI_END()


static PHP_GINIT_FUNCTION(fiber)
{
#if defined(ZTS) && defined(COMPILE_DL_FIBER)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	ZEND_SECURE_ZERO(fiber_globals, sizeof(zend_fiber_globals));

	zend_hash_init(&fiber_globals->fibers, 0, NULL, NULL, 1);
}

PHP_MINIT_FUNCTION(fiber)
{
	zend_fiber_ce_register();

	zend_observer_error_register(zend_fiber_error_observer);

	REGISTER_INI_ENTRIES();

	return SUCCESS;
}


PHP_MSHUTDOWN_FUNCTION(fiber)
{
	zend_fiber_ce_unregister();

	UNREGISTER_INI_ENTRIES();

	return SUCCESS;
}


static PHP_MINFO_FUNCTION(fiber)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "Fiber backend", zend_fiber_backend_info());
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}


static PHP_RINIT_FUNCTION(fiber)
{
#if defined(ZTS) && defined(COMPILE_DL_FIBER)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	zend_fiber_startup();

	return SUCCESS;
}

static PHP_RSHUTDOWN_FUNCTION(fiber)
{
	zend_fiber_shutdown();

	return SUCCESS;
}

static PHP_GSHUTDOWN_FUNCTION(fiber)
{
	zend_hash_destroy(&fiber_globals->fibers);
}

zend_module_entry fiber_module_entry = {
	STANDARD_MODULE_HEADER,
	"fiber",
	NULL,
	PHP_MINIT(fiber),
	PHP_MSHUTDOWN(fiber),
	PHP_RINIT(fiber),
	PHP_RSHUTDOWN(fiber),
	PHP_MINFO(fiber),
	PHP_FIBER_VERSION,
	PHP_MODULE_GLOBALS(fiber),
	PHP_GINIT(fiber),
	PHP_GSHUTDOWN(fiber),
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};


#ifdef COMPILE_DL_FIBER
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(fiber)
#endif

/*
 * vim: sw=4 ts=4
 * vim600: fdm=marker
 */
