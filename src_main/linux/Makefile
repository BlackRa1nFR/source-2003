#
# Half-life Makefile for x86 Linux
#
# Feb 2001 by Leon Hartwig (hartwig@valvesoftware.com)
#

SOURCE_DIR=..
BUILD_DIR=.

ENGINE_SRC_DIR=$(SOURCE_DIR)/engine
COMMON_SRC_DIR=$(SOURCE_DIR)/common
GAME_SHARED_SRC_DIR=$(SOURCE_DIR)/game_shared
PUBLIC_SRC_DIR=$(SOURCE_DIR)/public
DBG_SRC_DIR=$(SOURCE_DIR)/dbg
TIER0_SRC_DIR=$(SOURCE_DIR)/tier0

BUILD_OBJ_DIR=$(BUILD_DIR)/obj

ARCH=i486
# the symbol map file to tell the linker what to export
MAP=module.map

ifeq "$(HOSTNAME)" "alfred_linux.valvesoftware.com"
	CC=gcc
	CPLUS=g++
	CPP_LIB=-lstdc++
endif

ifeq "$(HOSTNAME)" "alfred"
	CC=/usr/bin/gcc
	CPLUS=/usr/bin/g++
	CPP_LIB = /usr/lib/gcc-lib/i586-mandrake-linux-gnu/2.96/libstdc++.a
endif

ifeq "$(HOSTNAME)" "linuxbuild2"
        CC=/usr/local/bin/gcc
        CPLUS=/usr/local/bin/g++
        CPP_LIB = /usr/local/lib/gcc-lib/i686-pc-linux-gnu/2.95.2/libstdc++.a
endif

ifeq "$(HOSTNAME)" "linuxbuild3"
        CC=/usr/bin/gcc
        CPLUS=/usr/bin/g++
        CPP_LIB = /usr/lib/gcc-lib/i586-mandrake-linux-gnu/2.96/libstdc++.a
endif


ifeq "$(HOSTNAME)" "alfred_linux"
	CC=gcc
	CPLUS=g++
	# this should be a .a file if we can find one...
	CPP_LIB = /usr/lib/gcc-lib/i586-mandrake-linux-gnu/3.2/libstdc++.so 
#else
#	CC=/usr/local/bin/gcc
#	CPLUS=/usr/local/bin/gcc
#	CPLUS=/usr/local/bin/g++
#	CPP_LIB = /usr/lib/gcc-lib/i486-linux/egcs-2.91.66/libstdc++.a 
#	CPP_LIB = /usr/local/lib/gcc-lib/i686-pc-linux-gnu/2.95.2/libstdc++.a
#	CPP_LIB = -lstdc++
endif

ARCH_CFLAGS_I486="-mcpu=i586 -mmmx -msse"
# -msse2"
#-malign-loops=4 -malign-jumps=4 -malign-functions=4"
ARCH_CFLAGS_I686="-march=i686 -mmmx -msse -msse2 -O3 -pipe -fomit-frame-pointer -ffast-math -fforce-addr -funroll-loops -malign-loops=4 -malign-jumps=4 -malign-functions=4" 
ARCH_CFLAGS_AMD="-march=k6 -mmmx -msse -msse2 -O3 -pipe -fomit-frame-pointer -ffast-math -fforce-addr -funroll-loops -malign-loops=4 -malign-jumps=4 -malign-functions=4"

ARCH_CFLAGS=$(ARCH_CFLAGS_I486)

BASE_CFLAGS="-D_LINUX -DNDEBUG -Dstricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_vsnprintf=vsnprintf"
#-pipe  -fno-exceptions -fno-rtti
SHLIBEXT=so
SHLIBCFLAGS=-fPIC
SHLIBLDFLAGS="-shared -Wl,-Map,$@_map.txt -Wl"
#,-Bsymbolic"
#,--version-script=$(MAP)"
ARCMD="ar rvs"
LIBEXT=a
CHECK_DSP="make_check/check_differences"

BASE_DEFINES_I486=CC=$(CC) CPLUS=$(CPLUS) CPP_LIB=$(CPP_LIB) BUILD_DIR=$(BUILD_DIR) BUILD_OBJ_DIR=$(BUILD_OBJ_DIR) SOURCE_DIR=$(SOURCE_DIR) ENGINE_SRC_DIR=$(ENGINE_SRC_DIR) COMMON_SRC_DIR=$(COMMON_SRC_DIR) BASE_CFLAGS=$(BASE_CFLAGS) PUBLIC_SRC_DIR=$(PUBLIC_SRC_DIR) DBG_SRC_DIR=$(DBG_SRC_DIR) ARCH_CFLAGS=$(ARCH_CFLAGS_I486) SHLIBLDFLAGS=$(SHLIBLDFLAGS) SHLIBEXT=$(SHLIBEXT) LIBEXT=$(LIBEXT) SHLIBLDFLAGS=$(SHLIBLDFLAGS) SHLIBEXT=$(SHLIBEXT) CHECK_DSP=$(CHECK_DSP) ARCMD=$(ARCMD)


#############################################################################
# PROJECT MAKEFILES
#############################################################################

MAKE_VSTDLIB=$(MAKE) -f Makefile.vstdlib
MAKE_TIER0=$(MAKE) -f Makefile.tier0
MAKE_UNITLIB=$(MAKE) -f Makefile.unitlib
MAKE_VTF=$(MAKE) -f Makefile.vtf
MAKE_IVP_PHYSICS=$(MAKE) -f ivp/Makefile.ivp_physics
MAKE_HK_BASE=$(MAKE) -f ivp/Makefile.hk_base
MAKE_HK_MATH=$(MAKE) -f ivp/Makefile.hk_math
MAKE_IVP_COMPACTBUILDER=$(MAKE) -f ivp/Makefile.ivp_compactbuilder
MAKE_HK_CONSTRAINT=$(MAKE) -f ivp/Makefile.hk_constraints
MAKE_VPHYSICS=$(MAKE) -f Makefile.vphysics
MAKE_STUDIO=$(MAKE) -f Makefile.studiorender
MAKE_MATSYSTEM=$(MAKE) -f Makefile.materialsystem
MAKE_SHADEREMPTY=$(MAKE) -f Makefile.shaderempty
MAKE_FS=$(MAKE) -f Makefile.stdio
MAKE_ENGINE=$(MAKE) -f Makefile.engine
MAKE_CSTRIKE=$(MAKE) -f Makefile.cs_dll
MAKE_DEDICATED=$(MAKE) -f Makefile.dedicated

#############################################################################
# SETUP AND BUILD
#############################################################################

all: build_dir targets

TARGETS=\
	tier0 \
	vstdlib \
	unitlib \
	vtf \
	ivp_physics \
	hk_base \
	hk_math \
	ivp_cb \
	hk_constraints \
	vphysics \
	studiorender \
	materialsystem \
	shaderempty \
	stdio \
	engine \
	cs \
	dedicated \

build_dir:
	if [ ! -d $(BUILD_DIR) ];then mkdir $(BUILD_DIR);fi
	cd $(BUILD_DIR)

targets: $(TARGETS)

vstdlib: tier0
	$(MAKE_VSTDLIB) ARCH=i486 $(BASE_DEFINES_I486)

tier0:
	$(MAKE_TIER0) ARCH=i486 $(BASE_DEFINES_I486)

unitlib:
	$(MAKE_UNITLIB) ARCH=i486 $(BASE_DEFINES_I486)

vtf: tier0 vstdlib
	$(MAKE_VTF) ARCH=i486 $(BASE_DEFINES_I486)

ivp_physics:
	$(MAKE_IVP_PHYSICS) ARCH=i486 $(BASE_DEFINES_I486)

hk_base:
	$(MAKE_HK_BASE) ARCH=i486  $(BASE_DEFINES_I486)

hk_math:
	$(MAKE_HK_MATH) ARCH=i486 $(BASE_DEFINES_I486)

ivp_cb:
	$(MAKE_IVP_COMPACTBUILDER) ARCH=i486  $(BASE_DEFINES_I486)

hk_constraints:
	$(MAKE_HK_CONSTRAINT) ARCH=i486  $(BASE_DEFINES_I486)

vphysics: hk_constraints ivp_cb hk_math hk_base ivp_physics tier0 vstdlib
	$(MAKE_VPHYSICS) ARCH=i486  $(BASE_DEFINES_I486)

studiorender: tier0 vstdlib
	$(MAKE_STUDIO) ARCH=i486 $(BASE_DEFINES_I486)

materialsystem: tier0 vstdlib vtf
	$(MAKE_MATSYSTEM) ARCH=i486 $(BASE_DEFINES_I486)

shaderempty: materialsystem
	$(MAKE_SHADEREMPTY) ARCH=i486 $(BASE_DEFINES_I486)

stdio: vstdlib tier0
	$(MAKE_FS) ARCH=i486 $(BASE_DEFINES_I486)

engine: stdio shaderempty vstdlib tier0
	$(MAKE_ENGINE) ARCH=i486 $(BASE_DEFINES_I486)

cs: engine
	$(MAKE_CSTRIKE) ARCH=i486 $(BASE_DEFINES_I486)

dedicated: tier0 vstdlib
	$(MAKE_DEDICATED) ARCH=i486 $(BASE_DEFINES_I486)

clean:
	$(MAKE_TIER0) ARCH=i486 LIBEXT=$(LIBEXT) BUILD_DIR=$(BUILD_DIR) SHLIBEXT=$(SHLIBEXT)  BUILD_OBJ_DIR=$(BUILD_OBJ_DIR) clean
	$(MAKE_VSTDLIB) ARCH=i486 LIBEXT=$(LIBEXT) BUILD_DIR=$(BUILD_DIR) SHLIBEXT=$(SHLIBEXT)  BUILD_OBJ_DIR=$(BUILD_OBJ_DIR) clean
	$(MAKE_UNITLIB) ARCH=i486 LIBEXT=$(LIBEXT) BUILD_DIR=$(BUILD_DIR) SHLIBEXT=$(SHLIBEXT)  BUILD_OBJ_DIR=$(BUILD_OBJ_DIR) clean
	$(MAKE_VTF) ARCH=i486 LIBEXT=$(LIBEXT) BUILD_DIR=$(BUILD_DIR) SHLIBEXT=$(SHLIBEXT)  BUILD_OBJ_DIR=$(BUILD_OBJ_DIR) clean
	$(MAKE_IVP_PHYSICS) ARCH=i486 LIBEXT=$(LIBEXT) BUILD_DIR=$(BUILD_DIR) SHLIBEXT=$(SHLIBEXT)   BUILD_OBJ_DIR=$(BUILD_OBJ_DIR) clean
	$(MAKE_HK_BASE) ARCH=i486 LIBEXT=$(LIBEXT) BUILD_DIR=$(BUILD_DIR) SHLIBEXT=$(SHLIBEXT)  BUILD_OBJ_DIR=$(BUILD_OBJ_DIR) clean
	$(MAKE_HK_MATH) ARCH=i486 LIBEXT=$(LIBEXT) BUILD_DIR=$(BUILD_DIR) SHLIBEXT=$(SHLIBEXT)  BUILD_OBJ_DIR=$(BUILD_OBJ_DIR) clean
	$(MAKE_IVP_COMPACTBUILDER) ARCH=i486 LIBEXT=$(LIBEXT) BUILD_DIR=$(BUILD_DIR) SHLIBEXT=$(SHLIBEXT)  BUILD_OBJ_DIR=$(BUILD_OBJ_DIR) clean
	$(MAKE_HK_CONSTRAINT) ARCH=i486 LIBEXT=$(LIBEXT)  BUILD_DIR=$(BUILD_DIR) SHLIBEXT=$(SHLIBEXT)  BUILD_OBJ_DIR=$(BUILD_OBJ_DIR) clean
	$(MAKE_VPHYSICS) ARCH=i486 LIBEXT=$(LIBEXT) BUILD_DIR=$(BUILD_DIR) SHLIBEXT=$(SHLIBEXT)  BUILD_OBJ_DIR=$(BUILD_OBJ_DIR) clean
	$(MAKE_STUDIO) ARCH=i486 LIBEXT=$(LIBEXT) BUILD_DIR=$(BUILD_DIR) SHLIBEXT=$(SHLIBEXT)  BUILD_OBJ_DIR=$(BUILD_OBJ_DIR) clean
	$(MAKE_MATSYSTEM) ARCH=i486 LIBEXT=$(LIBEXT) BUILD_DIR=$(BUILD_DIR) SHLIBEXT=$(SHLIBEXT)  BUILD_OBJ_DIR=$(BUILD_OBJ_DIR) clean
	$(MAKE_SHADEREMPTY) ARCH=i486 LIBEXT=$(LIBEXT) BUILD_DIR=$(BUILD_DIR) SHLIBEXT=$(SHLIBEXT)  BUILD_OBJ_DIR=$(BUILD_OBJ_DIR) clean
	$(MAKE_FS) ARCH=i486 LIBEXT=$(LIBEXT) BUILD_DIR=$(BUILD_DIR) SHLIBEXT=$(SHLIBEXT)  BUILD_OBJ_DIR=$(BUILD_OBJ_DIR) clean
	$(MAKE_ENGINE) ARCH=i486 LIBEXT=$(LIBEXT) BUILD_DIR=$(BUILD_DIR) SHLIBEXT=$(SHLIBEXT)  BUILD_OBJ_DIR=$(BUILD_OBJ_DIR) clean
	$(MAKE_CSTRIKE) ARCH=i486 LIBEXT=$(LIBEXT) BUILD_DIR=$(BUILD_DIR) SHLIBEXT=$(SHLIBEXT)  BUILD_OBJ_DIR=$(BUILD_OBJ_DIR) clean
	$(MAKE_DEDICATED) ARCH=i486 LIBEXT=$(LIBEXT) BUILD_DIR=$(BUILD_DIR) SHLIBEXT=$(SHLIBEXT)  BUILD_OBJ_DIR=$(BUILD_OBJ_DIR) clean
	-rm -rf $(BUILD_OBJ_DIR)
