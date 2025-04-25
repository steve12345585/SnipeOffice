# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozillarg/MPL/2.0/.
#

$(eval $(call gb_StaticLibrary_StaticLibrary,libjpeg-turbo))

$(eval $(call gb_StaticLibrary_use_unpacked,libjpeg-turbo,libjpeg-turbo))

$(eval $(call gb_StaticLibrary_set_warnings_disabled,libjpeg-turbo))

$(eval $(call gb_StaticLibrary_set_include,libjpeg-turbo,\
    -I$(gb_UnpackedTarball_workdir)/libjpeg-turbo \
    $$(INCLUDE) \
))

ifeq ($(OS),WNT)
$(eval $(call gb_StaticLibrary_add_cflags,libjpeg-turbo,\
    -DHAVE_INTRIN_H=1 \
))
endif

$(eval $(call gb_StaticLibrary_add_cflags,libjpeg-turbo,\
    -DSIZEOF_SIZE_T=$(SIZEOF_SIZE_T) \
))

ifeq ($(COM),GCC)
ifneq ($(ENABLE_OPTIMIZED),)
$(eval $(call gb_StaticLibrary_add_cflags,libjpeg-turbo,\
    -O3 \
))
endif
endif

$(eval $(call gb_StaticLibrary_add_generated_cobjects,libjpeg-turbo,\
    UnpackedTarball/libjpeg-turbo/src/jaricom \
    UnpackedTarball/libjpeg-turbo/src/jcapimin \
    UnpackedTarball/libjpeg-turbo/src/jcapistd \
    UnpackedTarball/libjpeg-turbo/src/jcarith \
    UnpackedTarball/libjpeg-turbo/src/jccoefct \
    UnpackedTarball/libjpeg-turbo/src/jccolor \
    UnpackedTarball/libjpeg-turbo/src/jcdctmgr \
    UnpackedTarball/libjpeg-turbo/src/jcdiffct \
    UnpackedTarball/libjpeg-turbo/src/jchuff \
    UnpackedTarball/libjpeg-turbo/src/jcicc \
    UnpackedTarball/libjpeg-turbo/src/jcinit \
    UnpackedTarball/libjpeg-turbo/src/jclhuff \
    UnpackedTarball/libjpeg-turbo/src/jclossls \
    UnpackedTarball/libjpeg-turbo/src/jcmainct \
    UnpackedTarball/libjpeg-turbo/src/jcmarker \
    UnpackedTarball/libjpeg-turbo/src/jcmaster \
    UnpackedTarball/libjpeg-turbo/src/jcomapi \
    UnpackedTarball/libjpeg-turbo/src/jcparam \
    UnpackedTarball/libjpeg-turbo/src/jcphuff \
    UnpackedTarball/libjpeg-turbo/src/jcprepct \
    UnpackedTarball/libjpeg-turbo/src/jcsample \
    UnpackedTarball/libjpeg-turbo/src/jctrans \
    UnpackedTarball/libjpeg-turbo/src/jdapimin \
    UnpackedTarball/libjpeg-turbo/src/jdapistd \
    UnpackedTarball/libjpeg-turbo/src/jdarith \
    UnpackedTarball/libjpeg-turbo/src/jdatadst \
    UnpackedTarball/libjpeg-turbo/src/jdatasrc \
    UnpackedTarball/libjpeg-turbo/src/jdcoefct \
    UnpackedTarball/libjpeg-turbo/src/jdcolor \
    UnpackedTarball/libjpeg-turbo/src/jddctmgr \
    UnpackedTarball/libjpeg-turbo/src/jddiffct \
    UnpackedTarball/libjpeg-turbo/src/jdhuff \
    UnpackedTarball/libjpeg-turbo/src/jdicc \
    UnpackedTarball/libjpeg-turbo/src/jdinput \
    UnpackedTarball/libjpeg-turbo/src/jdlhuff \
    UnpackedTarball/libjpeg-turbo/src/jdlossls \
    UnpackedTarball/libjpeg-turbo/src/jdmainct \
    UnpackedTarball/libjpeg-turbo/src/jdmarker \
    UnpackedTarball/libjpeg-turbo/src/jdmaster \
    UnpackedTarball/libjpeg-turbo/src/jdmerge \
    UnpackedTarball/libjpeg-turbo/src/jdphuff \
    UnpackedTarball/libjpeg-turbo/src/jdpostct \
    UnpackedTarball/libjpeg-turbo/src/jdsample \
    UnpackedTarball/libjpeg-turbo/src/jdtrans \
    UnpackedTarball/libjpeg-turbo/src/jerror \
    UnpackedTarball/libjpeg-turbo/src/jfdctflt \
    UnpackedTarball/libjpeg-turbo/src/jfdctfst \
    UnpackedTarball/libjpeg-turbo/src/jfdctint \
    UnpackedTarball/libjpeg-turbo/src/jidctflt \
    UnpackedTarball/libjpeg-turbo/src/jidctfst \
    UnpackedTarball/libjpeg-turbo/src/jidctint \
    UnpackedTarball/libjpeg-turbo/src/jidctred \
    UnpackedTarball/libjpeg-turbo/src/jmemmgr \
    UnpackedTarball/libjpeg-turbo/src/jmemnobs \
    UnpackedTarball/libjpeg-turbo/src/jpeg_nbits \
    UnpackedTarball/libjpeg-turbo/src/jquant1 \
    UnpackedTarball/libjpeg-turbo/src/jquant2 \
    UnpackedTarball/libjpeg-turbo/src/jutils \
))

ifneq ($(NASM),)

$(eval $(call gb_StaticLibrary_add_nasmflags,libjpeg-turbo,\
	-I$(gb_UnpackedTarball_workdir)/libjpeg-turbo/simd/nasm/ \
	-I$(dir $(gb_UnpackedTarball_workdir)/libjpeg-turbo/$(1)) \
))

ifeq ($(CPUNAME),X86_64)

$(eval $(call gb_StaticLibrary_add_cflags,libjpeg-turbo,\
    -DWITH_SIMD \
))

$(eval $(call gb_StaticLibrary_add_generated_cobjects,libjpeg-turbo,\
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jsimd \
))

$(eval $(call gb_StaticLibrary_add_generated_nasmobjects,libjpeg-turbo,\
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jsimdcpu.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jfdctflt-sse.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jccolor-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jcgray-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jchuff-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jcphuff-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jcsample-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jdcolor-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jdmerge-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jdsample-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jfdctfst-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jfdctint-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jidctflt-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jidctfst-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jidctint-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jidctred-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jquantf-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jquanti-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jccolor-avx2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jcgray-avx2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jcsample-avx2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jdcolor-avx2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jdmerge-avx2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jdsample-avx2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jfdctint-avx2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jidctint-avx2.asm \
    UnpackedTarball/libjpeg-turbo/simd/x86_64/jquanti-avx2.asm \
))

else ifeq ($(CPUNAME),INTEL)

$(eval $(call gb_StaticLibrary_add_cflags,libjpeg-turbo,\
    -DWITH_SIMD \
))

$(eval $(call gb_StaticLibrary_add_generated_cobjects,libjpeg-turbo,\
    UnpackedTarball/libjpeg-turbo/simd/i386/jsimd \
))

$(eval $(call gb_StaticLibrary_add_generated_nasmobjects,libjpeg-turbo,\
    UnpackedTarball/libjpeg-turbo/simd/i386/jsimdcpu.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jfdctflt-3dn.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jidctflt-3dn.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jquant-3dn.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jccolor-mmx.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jcgray-mmx.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jcsample-mmx.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jdcolor-mmx.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jdmerge-mmx.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jdsample-mmx.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jfdctfst-mmx.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jfdctint-mmx.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jidctfst-mmx.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jidctint-mmx.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jidctred-mmx.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jquant-mmx.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jfdctflt-sse.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jidctflt-sse.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jquant-sse.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jccolor-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jcgray-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jchuff-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jcphuff-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jcsample-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jdcolor-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jdmerge-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jdsample-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jfdctfst-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jfdctint-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jidctflt-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jidctfst-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jidctint-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jidctred-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jquantf-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jquanti-sse2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jccolor-avx2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jcgray-avx2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jcsample-avx2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jdcolor-avx2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jdmerge-avx2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jdsample-avx2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jfdctint-avx2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jidctint-avx2.asm \
    UnpackedTarball/libjpeg-turbo/simd/i386/jquanti-avx2.asm \
))

endif
endif


# vim: set noet sw=4 ts=4:
