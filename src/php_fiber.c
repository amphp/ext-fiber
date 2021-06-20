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

#include "php.h"
#include "ext/standard/info.h"

#include "php_fiber.h"
#include "fiber.h"

ZEND_DECLARE_MODULE_GLOBALS(fiber)

static PHP_INI_MH(OnUpdateFiberStackSize)
{
	if (new_value) {
		FIBER_G(stack_size) = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
	} else {
		FIBER_G(stack_size) = ZEND_FIBER_DEFAULT_C_STACK_SIZE;
	}
	return SUCCESS;
}

PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("fiber.stack_size", NULL, PHP_INI_ALL, OnUpdateFiberStackSize, stack_size, zend_fiber_globals, fiber_globals)
PHP_INI_END()


static PHP_GINIT_FUNCTION(fiber)
{
#if defined(ZTS) && defined(COMPILE_DL_FIBER)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	memset(fiber_globals, 0, sizeof(zend_fiber_globals));
}

PHP_MINIT_FUNCTION(fiber)
{
	zend_register_fiber_ce();

	REGISTER_INI_ENTRIES();

	return SUCCESS;
}


PHP_MSHUTDOWN_FUNCTION(fiber)
{
	zend_fiber_shutdown();

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

	zend_fiber_init();

	return SUCCESS;
}

static PHP_RSHUTDOWN_FUNCTION(fiber)
{
	return SUCCESS;
}

static PHP_GSHUTDOWN_FUNCTION(fiber)
{
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
