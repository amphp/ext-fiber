PHP_ARG_ENABLE(fiber, whether to enable fiber support,
[  --enable-fiber          Enable fiber fiber support], no)

if test "$PHP_FIBER" != "no"; then
  AC_DEFINE(HAVE_FIBER, 1, [ ])
  
  FIBER_CFLAGS="-Wall -Werror -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1"

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
    [x86*|amd*|i[[3456]]86*|pentium], [fiber_cpu="x86"],
    [aarch64*], [fiber_cpu="arm64"],
    [arm*], [fiber_cpu="arm"],
    [ppc64*], [fiber_cpu="ppc64"],
    [powerpc*], [fiber_cpu="ppc"],
    [mips64*], [fiber_cpu="mips64"],
    [mips*], [fiber_cpu="mips"],
    [fiber_cpu="unknown"]
  )
  
  AS_CASE([$host_os],
    [darwin*], [fiber_os="mac"],
    [cygwin*], [fiber_os="win"],
    [mingw*], [fiber_os="win"],
    [fiber_os="linux"]
  )

  AS_CASE([$fiber_cpu],
    [x86_64], [fiber_asm_file_prefix="x86_64_sysv"],
    [x86], [fiber_asm_file_prefix="x86_sysv"],
    [arm64], [fiber_asm_file_prefix="arm64_aapcs"],
    [arm], [fiber_asm_file_prefix="arm_aapcs"],
    [ppc64], [fiber_asm_file_prefix="ppc64_sysv"],
    [ppc], [fiber_asm_file_prefix="ppc_sysv"],
    [mips64], [fiber_asm_file_prefix="mips64_n64"],
    [mips], [fiber_asm_file_prefix="mips32_o32"],
    [fiber_asm_file_prefix="combined_sysv"]
  )

  if test "$fiber_os" = 'linux'; then
    fiber_asm_file="${fiber_asm_file_prefix}_elf_gas.S"
  elif test "$fiber_os" = 'mac'; then
    fiber_asm_file="${fiber_asm_file_prefix}_macho_gas.S"
  else
    fiber_use_asm="no"
  fi

  if test "$fiber_use_asm" = 'yes'; then
    fiber_source_files="$fiber_source_files \
      src/fiber_asm.c \
      boost/asm/make_${fiber_asm_file} \
      boost/asm/jump_${fiber_asm_file}"
  elif test "$fiber_use_ucontext" = 'yes'; then
    fiber_source_files="$fiber_source_files \
      src/fiber_ucontext.c"
  else
    AC_MSG_ERROR([No fiber context switching available!])
  fi
  
  PHP_NEW_EXTENSION(fiber, $fiber_source_files, $ext_shared,, \\$(FIBER_CFLAGS))
  PHP_SUBST(FIBER_CFLAGS)
  PHP_ADD_MAKEFILE_FRAGMENT
  
  PHP_INSTALL_HEADERS([ext/fiber], [config.h include/*.h])
fi
