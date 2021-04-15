/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: b3cdf5b0ae78c132a60852e4a4d059ac41b057fa */

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

#define arginfo_class_ReflectionFiber_isStarted arginfo_class_Fiber_isStarted

#define arginfo_class_ReflectionFiber_isSuspended arginfo_class_Fiber_isStarted

#define arginfo_class_ReflectionFiber_isRunning arginfo_class_Fiber_isStarted

#define arginfo_class_ReflectionFiber_isTerminated arginfo_class_Fiber_isStarted

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_FiberError___construct, 0, 0, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_FiberExit___construct arginfo_class_FiberError___construct
