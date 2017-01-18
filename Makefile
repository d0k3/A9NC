# TARGET #

TARGET := 3DS
LIBRARY := 0

ifeq ($(TARGET),3DS)
    ifeq ($(strip $(DEVKITPRO)),)
        $(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro")
    endif

    ifeq ($(strip $(DEVKITARM)),)
        $(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
    endif
endif

# COMMON CONFIGURATION #

NAME := A9NC

BUILD_DIR := build
OUTPUT_DIR := output
INCLUDE_DIRS := include
SOURCE_DIRS := source

EXTRA_OUTPUT_FILES :=

LIBRARY_DIRS := $(DEVKITPRO)/libctru $(DEVKITPRO)/portlibs/armv6k $(PORTLIBS)
LIBRARIES := ctru m z

BUILD_FLAGS := -DLIBKHAX_AS_LIB -DVERSION_STRING="\"`git describe --tags --abbrev=0`\""
RUN_FLAGS :=

OUTPUT_ZIP_FILE := $(OUTPUT_DIR)/$(NAME)-$(shell date +'%Y%m%d-%H%M%S').zip

# 3DS CONFIGURATION #

TITLE := $(NAME)
DESCRIPTION := ARM9 companion tool to receive payloads over wifi.
AUTHOR := d0k3
PRODUCT_CODE := CTR-P-A9NC
UNIQUE_ID := 0x2871

SYSTEM_MODE := 64MB
SYSTEM_MODE_EXT := Legacy

ICON_FLAGS :=

ROMFS_DIR := 
BANNER_AUDIO := meta/audio.wav
BANNER_IMAGE := meta/banner.png
ICON := meta/icon.png
LOGO := meta/logo.bcma.lz

# INTERNAL #

include buildtools/make_base