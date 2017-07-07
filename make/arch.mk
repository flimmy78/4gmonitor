#/opt/au/qca/qsdk/staging_dir/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2/bin/
#CROSSTOOLDIR :=/opt/au/qca/qsdk
#export	STAGING_DIR	:= $(CROSSTOOLDIR)/staging_dir
#export	PATH :=$(PATH):$(STAGING_DIR)/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2/bin
#CROSS 	?= mips-openwrt-linux-
CROSS 	?= mipsel-openwrt-linux-
ARCH		?= mips

GCC 		?= $(CROSS)gcc
CXX			?= $(CROSS)g++
AR			?= $(CROSS)ar
AS			?= $(CROSS)gcc
RANLIB	?= $(CROSS)ranlib
STRIP 	?= $(CROSS)strip
OBJCOPY	?= $(CROSS)objcopy
OBJDUMP ?= $(CROSS)objdump
SIZE		?= $(CROSS)size
LD			?= $(CROSS)ld
MKDIR		?= mkdir -p


TARGET_CFLAGS 		+= -Wall -g -O2 -I$(ROOTDIR)/inc -I$(ROOTDIR)/inc/ayla -I$(ROOTDIR)/platform -I$(ROOTDIR)/product/serial/inc 

TARGET_CXXFLAGS 	+= $(TARGET_CFLAGS) -std=c++0x

#TARGET_LDFLAGS 		+= -L$(ROOTDIR)/lib -lm -lrt -ldl -lpthread -lubus -lblobmsg_json -lubox
TARGET_LDFLAGS 		+= -L$(ROOTDIR)/lib -lm -lrt -ldl -lpthread
#TARGET_LDFLAGS 	+= -L/usr/lib/ -ljansson
#TARGET_LDFLAGS		+= -lstdc++

