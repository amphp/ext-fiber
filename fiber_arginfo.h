/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 00a3e9dd5ec88c8acd84d1d84c6b6e7702d6a99a */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Fiber___construct, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, callback, IS_CALLABLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Fiber_start, 0, 0, IS_MIXED, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, args, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Fiber_resume, 0, 0, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, value, IS_MIXED, 0, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Fiber_throw, 0, 1, IS_MIXED, 0)
	ZEND_ARG_OBJ_INFO(0, exception, Throwable, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Fiber_isStarted, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Fiber_isSuspended arginfo_class_Fiber_isStarted

#define arginfo_class_Fiber_isRunning arginfo_class_Fiber_isStarted

#define arginfo_class_Fiber_isTerminated arginfo_class_Fiber_isStarted

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Fiber_getReturn, 0, 0, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_Fiber_this, 0, 0, Fiber, 1)
ZEND_END_ARG_INFO()

#define arginfo_class_Fiber_suspend arginfo_class_Fiber_resume

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_ReflectionFiber___construct, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, fiber, Fiber, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_ReflectionFiber_getFiber, 0, 0, Fiber, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_ReflectionFiber_getExecutingFile, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_ReflectionFiber_getExecutingLine, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_ReflectionFiber_getTrace, 0, 0, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, options, IS_LONG, 0, "DEBUG_BACKTRACE_PROVIDE_OBJECT")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_ReflectionFiber_getCallable, 0, 0, IS_CALLABLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_FiberError___construct, 0, 0, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FiberExit___construct arginfo_class_FiberError___construct


ZEND_METHOD(Fiber, __construct);
ZEND_METHOD(Fiber, start);
ZEND_METHOD(Fiber, resume);
ZEND_METHOD(Fiber, throw);
ZEND_METHOD(Fiber, isStarted);
ZEND_METHOD(Fiber, isSuspended);
ZEND_METHOD(Fiber, isRunning);
ZEND_METHOD(Fiber, isTerminated);
ZEND_METHOD(Fiber, getReturn);
ZEND_METHOD(Fiber, this);
ZEND_METHOD(Fiber, suspend);
ZEND_METHOD(ReflectionFiber, __construct);
ZEND_METHOD(ReflectionFiber, getFiber);
ZEND_METHOD(ReflectionFiber, getExecutingFile);
ZEND_METHOD(ReflectionFiber, getExecutingLine);
ZEND_METHOD(ReflectionFiber, getTrace);
ZEND_METHOD(ReflectionFiber, getCallable);
ZEND_METHOD(FiberError, __construct);
ZEND_METHOD(FiberExit, __construct);


static const zend_function_entry class_Fiber_methods[] = {
	ZEND_ME(Fiber, __construct, arginfo_class_Fiber___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, start, arginfo_class_Fiber_start, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, resume, arginfo_class_Fiber_resume, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, throw, arginfo_class_Fiber_throw, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, isStarted, arginfo_class_Fiber_isStarted, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, isSuspended, arginfo_class_Fiber_isSuspended, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, isRunning, arginfo_class_Fiber_isRunning, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, isTerminated, arginfo_class_Fiber_isTerminated, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, getReturn, arginfo_class_Fiber_getReturn, ZEND_ACC_PUBLIC)
	ZEND_ME(Fiber, this, arginfo_class_Fiber_this, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(Fiber, suspend, arginfo_class_Fiber_suspend, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_FE_END
};


static const zend_function_entry class_ReflectionFiber_methods[] = {
	ZEND_ME(ReflectionFiber, __construct, arginfo_class_ReflectionFiber___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(ReflectionFiber, getFiber, arginfo_class_ReflectionFiber_getFiber, ZEND_ACC_PUBLIC)
	ZEND_ME(ReflectionFiber, getExecutingFile, arginfo_class_ReflectionFiber_getExecutingFile, ZEND_ACC_PUBLIC)
	ZEND_ME(ReflectionFiber, getExecutingLine, arginfo_class_ReflectionFiber_getExecutingLine, ZEND_ACC_PUBLIC)
	ZEND_ME(ReflectionFiber, getTrace, arginfo_class_ReflectionFiber_getTrace, ZEND_ACC_PUBLIC)
	ZEND_ME(ReflectionFiber, getCallable, arginfo_class_ReflectionFiber_getCallable, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};


static const zend_function_entry class_FiberError_methods[] = {
	ZEND_ME(FiberError, __construct, arginfo_class_FiberError___construct, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};


static const zend_function_entry class_FiberExit_methods[] = {
	ZEND_ME(FiberExit, __construct, arginfo_class_FiberExit___construct, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static zend_class_entry *register_class_Fiber(void)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "Fiber", class_Fiber_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	class_entry->ce_flags |= ZEND_ACC_FINAL|ZEND_ACC_NO_DYNAMIC_PROPERTIES;

	return class_entry;
}

static zend_class_entry *register_class_ReflectionFiber(void)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "ReflectionFiber", class_ReflectionFiber_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	class_entry->ce_flags |= ZEND_ACC_FINAL;

	return class_entry;
}

static zend_class_entry *register_class_FiberError(zend_class_entry *class_entry_Error)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "FiberError", class_FiberError_methods);
	class_entry = zend_register_internal_class_ex(&ce, class_entry_Error);
	class_entry->ce_flags |= ZEND_ACC_FINAL;

	return class_entry;
}

static zend_class_entry *register_class_FiberExit(zend_class_entry *class_entry_Exception)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "FiberExit", class_FiberExit_methods);
	class_entry = zend_register_internal_class_ex(&ce, class_entry_Exception);
	class_entry->ce_flags |= ZEND_ACC_FINAL;

	return class_entry;
}
