PHP_ARG_ENABLE(fiber, whether to enable fiber support,
[  --enable-fiber          Enable fiber fiber support], no)

if test "$PHP_FIBER" != "no"; then
  AC_DEFINE(HAVE_FIBER, 1, [ ])
  
  FIBER_CFLAGS="-Wall -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1"

  fiber_source_files="src/php_fiber.c \
    src/fiber.c \
    src/fiber_stack.c"
  
  fiber_use_asm="yes"
  fiber_user_ucontext="no"
  
  AC_CHECK_HEADER(ucontext.h, [
    fiber_use_ucontext="yes"
  ])
  
  AS_CASE([$host_cpu],
    [x86_64*], [fiber_cpu="x86_64"],
    [x86*], [fiber_cpu="x86"],
    [arm*], [fiber_cpu="arm"],
    [arm64*], [fiber_cpu="arm64"],
    [fiber_cpu="unknown"]
  )
  
  AS_CASE([$host_os],
    [darwin*], [fiber_os="MAC"],
    [cygwin*], [fiber_os="WIN"],
    [mingw*], [fiber_os="WIN"],
    [fiber_os="LINUX"]
  )
  
  if test "$fiber_cpu" = 'x86_64'; then
    if test "$fiber_os" = 'LINUX'; then
      fiber_asm_file="x86_64_sysv_elf_gas.S"
    elif test "$fiber_os" = 'MAC'; then
      fiber_asm_file="x86_64_sysv_macho_gas.S"
    else
      fiber_use_asm="no"
    fi
  elif test "$fiber_cpu" = 'x86'; then
    if test "$fiber_os" = 'LINUX'; then
      fiber_asm_file="i386_sysv_elf_gas.S"
    elif test "$fiber_os" = 'MAC'; then
      fiber_asm_file="i386_sysv_macho_gas.S"
    else
      fiber_use_asm="no"
    fi
  elif test "$fiber_cpu" = 'arm'; then
    if test "$fiber_os" = 'LINUX'; then
      fiber_asm_file="arm_aapcs_elf_gas.S"
    elif test "$fiber_os" = 'MAC'; then
      fiber_asm_file="arm_aapcs_macho_gas.S"
    else
      fiber_use_asm="no"
    fi
  else
    fiber_use_asm="no"
  fi
  
  if test "$fiber_use_asm" = 'yes'; then
    fiber_source_files="$fiber_source_files \
      src/fiber_asm.c \
      boost/asm/make_${fiber_asm_file} \
      boost/asm/jump_${fiber_asm_file}"
  elif test "$task_use_ucontext" = 'yes'; then
      task_source_files="$task_source_files \
        src/fiber_ucontext.c"
  fi
  
  PHP_NEW_EXTENSION(fiber, $fiber_source_files, $ext_shared,, \\$(FIBER_CFLAGS))
  PHP_SUBST(FIBER_CFLAGS)
  PHP_ADD_MAKEFILE_FRAGMENT
  
  PHP_INSTALL_HEADERS([ext/fiber], [config.h include/*.h])
fi
