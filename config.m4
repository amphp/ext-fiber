PHP_ARG_ENABLE([fiber],
  [whether to enable fiber support],
  [AS_HELP_STRING([--enable-fiber],
    [Enable fiber support])], [no])

if test "$PHP_FIBER" != "no"; then
  AC_DEFINE(HAVE_FIBER, 1, [ ])

  FIBER_CFLAGS="-Wall -Werror -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1"

  fiber_source_files="src/php_fiber.c \
    src/fiber.c \
    src/fiber_stack.c"

  AC_MSG_CHECKING(for fiber switching context)
  fiber="yes"

  AS_CASE([$host_cpu],
    [x86_64*|amd64*], [fiber_cpu="x86_64"],
    [x86*|amd*|i?86*|pentium], [fiber_cpu="i386"],
    [aarch64*|arm64*], [fiber_cpu="arm64"],
    [arm*], [fiber_cpu="arm32"],
    [ppc64*|powerpc64*], [fiber_cpu="ppc64"],
    [ppc*|powerpc*], [fiber_cpu="ppc32"],
    [s390x*], [fiber_cpu="s390x"],
    [mips64*], [fiber_cpu="mips64"],
    [mips*], [fiber_cpu="mips32"],
    [fiber_cpu="unknown"]
  )

  AS_CASE([$host_os],
    [darwin*], [fiber_os="mac"],
    [fiber_os="other"]
  )

  AS_CASE([$fiber_cpu],
    [x86_64], [fiber_asm_file_prefix="x86_64_sysv"],
    [i386], [fiber_asm_file_prefix="i386_sysv"],
    [arm64], [fiber_asm_file_prefix="arm64_aapcs"],
    [arm32], [fiber_asm_file_prefix="arm_aapcs"],
    [ppc64], [fiber_asm_file_prefix="ppc64_sysv"],
    [ppc32], [fiber_asm_file_prefix="ppc32_sysv"],
    [s390x], [fiber_asm_file_prefix="s390x_sysv"],
    [mips64], [fiber_asm_file_prefix="mips64_n64"],
    [mips32], [fiber_asm_file_prefix="mips32_o32"],
    [fiber_asm_file_prefix="unknown"]
  )

  if test "$fiber_os" = 'mac'; then
    fiber_asm_file="combined_sysv_macho_gas"
  elif test "$fiber_asm_file_prefix" != 'unknown'; then
    fiber_asm_file="${fiber_asm_file_prefix}_elf_gas"
  else
    fiber="no"
  fi

  if test "$fiber" = 'yes'; then
    fiber_source_files="$fiber_source_files \
      src/fiber_asm.c \
      boost/asm/make_${fiber_asm_file}.S \
      boost/asm/jump_${fiber_asm_file}.S"
    AC_MSG_RESULT([$fiber_asm_file])
  else
    AC_MSG_ERROR([Unable to determine platform for fiber switching context!])
  fi

  PHP_NEW_EXTENSION(fiber, $fiber_source_files, $ext_shared,, \\$(FIBER_CFLAGS))
  PHP_SUBST(FIBER_CFLAGS)
  PHP_ADD_MAKEFILE_FRAGMENT
  
  PHP_INSTALL_HEADERS([ext/fiber], [php_fiber.h fiber.h])
fi
