###########################################################                                                                         # Copyright (C) 2015-2020 Hisilion
# config.mk
###########################################################

#define build parameter
MAKE_PARAMETER := -j 32

#define build varibales
TOPDIR        := $(FLYFISH_TOP)
BUILD_SYSTEM  := $(TOPDIR)/build
OUTDIR        := $(TOPDIR)/target
HOST_OUTDIR   := $(OUTDIR)/host
PLATFORM_DIR  := $(FLYFISH_SDK_PATH)
FRAMEWORK_DIR := $(TOPDIR)/framework
PROJECT_DIR   := $(TOPDIR)/project

##########################################################
# define build template variable
##########################################################
BUILD_STATIC_LIBRARY      := $(BUILD_SYSTEM)/static_library.mk
BUILD_SHARED_LIBRARY      := $(BUILD_SYSTEM)/shared_library.mk
BUILD_EXECUTABLE          := $(BUILD_SYSTEM)/executable.mk
BUILD_HOST_EXECUTABLE     := $(BUILD_SYSTEM)/host_executable.mk
BUILD_HOST_SHARED_LIBRARY := $(BUILD_SYSTEM)/host_shared_library.mk
BUILD_HOST_STATIC_LIBRARY := $(BUILD_SYSTEM)/host_static_library.mk
BUILD_OPENSOURCE_PKG      := $(BUILD_SYSTEM)/opensource_pkg.mk
BUILD_SELFDEFINE_PKG      := $(BUILD_SYSTEM)/selfdefine_pkg.mk
#BUILD_PREBUILT := $(BUILD_SYSTEM)/prebuilt.mk
CLEAR_VARS                := $(BUILD_SYSTEM)/clear_vars.mk

##########################################################
# build toolchain config
# varibale CROSS value depend on user config from menuconfig
##########################################################
export CROSS:=$(CFG_TOOLCHAINS_NAME)

ifeq ($(CFG_TOOLCHAINS_NAME),arm-hisiv200-linux)
    USER_CFLAGS = -march=armv7-a -mcpu=cortex-a9 -mfloat-abi=softfp -mfpu=vfpv3-d16
endif

ifeq ($(CFG_TOOLCHAINS_NAME),arm-histbv310-linux)
ifeq (${CFG_CHIP_TYPE},hi3798mv100)
    USER_CFLAGS = -mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=vfpv4-d16
else
ifeq (${CFG_CHIP_TYPE},hi3796mv100)
    USER_CFLAGS = -mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=vfpv4-d16
else
ifeq (${CFG_CHIP_TYPE},hi3716dv100)
    USER_CFLAGS = -mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=vfpv4-d16
else
    USER_CFLAGS = -march=armv7-a -mcpu=cortex-a9 -mfloat-abi=softfp -mfpu=vfpv3-d16
endif
endif
endif
endif
#comile cmd for target machine

TARGET_AR      := $(CROSS)-ar
TARGET_AS      := $(CROSS)-as
TARGET_LD      := $(CROSS)-ld
TARGET_CXX     := $(CROSS)-g++
TARGET_CC      := $(CROSS)-gcc
TARGET_NM      := $(CROSS)-nm
TARGET_STRIP   := $(CROSS)-strip
TARGET_OBJCOPY := $(CROSS)-objcopy
TARGET_OBJDUMP := $(CROSS)-objdump
TARGET_RANLIB  := $(CROSS)-ranlib

TARGET_GLOBLE_CFLAGS     := -Wall -O2 -fPIC -DLINUX_OS -DTVOS_H -ffunction-sections -fdata-sections -Wl,--gc-sections
TARGET_GLOBLE_CPPFLAGS   := -Wall -O2 -fPIC -DLINUX_OS -DTVOS_H -ffunction-sections -fdata-sections -Wl,--gc-sections
TARGET_GLOBLE_LDFLAGS    :=
TARGET_GLOBLE_ARFLAGS    := rcs
TARGET_RELEASE_CPPFLAGS  :=

TARGET_C_INCLUDES        :=
TARGET_DEFAULT_SYSTEM_SHARED_LIBRARIES :=

TARGET_EXECUTABLE_SUFFIX :=
TARGET_SHAREDLIB_SUFFIX  :=.so
TARGET_STATICLIB_SUFFIX  :=.a


#compile cmd for host machine

HOST_AR      := ar
HOST_AS      := as
HOST_LD      := ld
HOST_CXX     := g++
HOST_CC      := gcc
HOST_NM      := nm
HOST_STRIP   := strip
HOST_OBJCOPY := objcopy
HOST_OBJDUMP := objdump
HOST_RANLIB  := ranlib

HOST_GLOBLE_CFLAGS     := -fPIC -Wall -ffunction-sections -fdata-sections -Wl,--gc-sections
HOST_GLOBLE_CPPFLAGS   := -fPIC -Wall -ffunction-sections -fdata-sections -Wl,--gc-sections
HOST_GLOBLE_LDFLAGS    :=
HOST_GLOBLE_ARFLAGS    := rcs
HOST_EXECUTABLE_SUFFIX :=
HOST_SHAREDLIB_SUFFIX  :=.so
HOST_STATICLIB_SUFFIX  :=.a

#####################################################
# target output dirs
#####################################################
IMAGE_DIR    := $(OUTDIR)/image
INCLUDE_DIR  := $(OUTDIR)/include
LIB_DIR      := $(OUTDIR)/lib
BIN_DIR      := $(OUTDIR)/bin
CONFIG_DIR   := $(OUTDIR)/config
OBJ_DIR      := $(OUTDIR)/obj
ROOTBOX_DIR  := $(IMAGE_DIR)/rootbox

HOST_BIN_DIR := $(HOST_OUTDIR)/bin
HOST_LIB_DIR := $(HOST_OUTDIR)/lib
HOST_INCLUDE := $(HOST_OUTDIR)/include
HOST_OBJ_DIR := $(HOST_OUTDIR)/obj

#####################################################
# sdk directory
#####################################################
SDK_PUB_DIR      := $(PLATFORM_DIR)/$(CFG_SDK_TYPE)/pub
SDK_IMAGE_DIR    := $(PLATFORM_DIR)/$(CFG_SDK_TYPE)/pub/image
SDK_TOOLS_DIR    := $(PLATFORM_DIR)/$(CFG_SDK_TYPE)/tools/linux
SDK_SERVER_UNTILS_DIR := ${SDK_TOOLS_DIR}/utils

#####################################################
# Image parameter config
#####################################################
EMMC_SIZE     :=
EMMC_SIZE_STR := 256M

#####################################################
# SERVER_UNTILS
#####################################################
TOOL_MKIMAGE     :=${SDK_SERVER_UNTILS_DIR}/mkimage
TOOL_MKCRAMFS    :=${SDK_SERVER_UNTILS_DIR}/mkfs.cramfs
TOOL_MKCRAMFS    :=${SDK_SERVER_UNTILS_DIR}/mkfs.cramfs
TOOL_MKEXT2FS    :=${SDK_SERVER_UNTILS_DIR}/genext2fs
TOOL_MKEXT4FS    :=${SDK_SERVER_UNTILS_DIR}/make_ext4fs
TOOL_TUNE2FS     :=${SDK_SERVER_UNTILS_DIR}/tune2fs
TOOL_E2FSCK      :=${SDK_SERVER_UNTILS_DIR}/e2fsck
TOOL_MKSQUASHFS  :=${SDK_SERVER_UNTILS_DIR}/mksquashfs
TOOL_MKJFFS2     :=${SDK_SERVER_UNTILS_DIR}/mkfs.jffs2
TOOL_MKYAFFS     :=${SDK_SERVER_UNTILS_DIR}/mkyaffs2image610
TOOL_MKUBIIMG    :=${SDK_SERVER_UNTILS_DIR}/mkubiimg.sh
TOOL_FILECAP     :=${SDK_SERVER_UNTILS_DIR}/filecap
TOOL_SCP         :=${SDK_SERVER_UNTILS_DIR}/cp
TOOL_SRM         :=${SDK_SERVER_UNTILS_DIR}/rm

#####################################################
# target link dirs
#####################################################
TARGET_GLOBAL_LD_DIRS        :=$(LIB_DIR)
TARGET_GLOBAL_LD_SHARED_DIRS :=$(LIB_DIR)/share
TARGET_GLOBAL_LD_STATIC_DIRS :=$(LIB_DIR)/static
TARGET_GLOBAL_LD_EXTERN_DIRS :=$(LIB_DIR)/extern

HOST_GLOBAL_LD_DIRS          :=$(HOST_LIB_DIR)
HOST_GLOBAL_LD_SHARED_DIRS   :=$(HOST_LIB_DIR)/share
HOST_GLOBAL_LD_STATIC_DIRS   :=$(HOST_LIB_DIR)/static
HOST_GLOBAL_LD_EXTERN_DIRS   :=$(HOST_LIB_DIR)/extern

TARGET_C_INCLUDES :=
TARGET_BIN_DIR    :=$(BIN_DIR)
TARGET_CONFIG_DIR :=$(CONFIG_DIR)

####################################################
# common shell cmds
####################################################
MKDIR :=mkdir
CP :=cp
RM :=rm
TOUCH := touch
$(shell mkdir -p $(TOPDIR)/target/include)
$(shell mkdir -p $(TOPDIR)/target/lib)
$(shell mkdir -p $(TOPDIR)/target/lib/static)
$(shell mkdir -p $(TOPDIR)/target/lib/share)
$(shell mkdir -p $(TOPDIR)/target/lib/extern)
$(shell mkdir -p $(TOPDIR)/target/image)
$(shell mkdir -p $(TOPDIR)/target/config)
