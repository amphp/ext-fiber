/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2018 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Martin Schröder <m.schroeder2007@gmail.com>                 |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"

#include "php_task.h"
#include "fiber.h"

ZEND_DECLARE_MODULE_GLOBALS(task)

static PHP_INI_MH(OnUpdateFiberStackSize)
{
	OnUpdateLong(entry, new_value, mh_arg1, mh_arg2, mh_arg3, stage);

	if (TASK_G(stack_size) < 0) {
		TASK_G(stack_size) = 0;
	}

	return SUCCESS;
}

PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("task.stack_size", "0", PHP_INI_SYSTEM, OnUpdateFiberStackSize, stack_size, zend_task_globals, task_globals)
PHP_INI_END()


static PHP_GINIT_FUNCTION(task)
{
#if defined(ZTS) && defined(COMPILE_DL_TASK)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	ZEND_SECURE_ZERO(task_globals, sizeof(zend_task_globals));
}

PHP_MINIT_FUNCTION(task)
{
	zend_fiber_ce_register();

	REGISTER_INI_ENTRIES();

	return SUCCESS;
}


PHP_MSHUTDOWN_FUNCTION(task)
{
	zend_fiber_ce_unregister();

	UNREGISTER_INI_ENTRIES();

	return SUCCESS;
}


static PHP_MINFO_FUNCTION(task)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "Fiber backend", "asm");
	php_info_print_table_row(2, "Boost Context version", "1.67");
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}


static PHP_RINIT_FUNCTION(task)
{
#if defined(ZTS) && defined(COMPILE_DL_TASK)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	return SUCCESS;
}

static PHP_RSHUTDOWN_FUNCTION(task)
{
	zend_fiber_shutdown();

	return SUCCESS;
}

zend_module_entry task_module_entry = {
	STANDARD_MODULE_HEADER,
	"task",
	NULL,
	PHP_MINIT(task),
	PHP_MSHUTDOWN(task),
	PHP_RINIT(task),
	PHP_RSHUTDOWN(task),
	PHP_MINFO(task),
	PHP_TASK_VERSION,
	PHP_MODULE_GLOBALS(task),
	PHP_GINIT(task),
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};


#ifdef COMPILE_DL_TASK
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(task)
#endif

/*
 * vim: sw=4 ts=4
 * vim600: fdm=marker
 */
