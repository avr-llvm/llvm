# This file is included by makefiles in board specific subdirectories
F_USB        = $(F_CPU)
ARCH         = AVR8
OPTIMIZATION = s
TARGET       = avrlit
LUFA_PATH    = ../lufa/LUFA
SRC          = ../src/avrlit.cpp ../src/usb.c $(LUFA_SRC_USB_DEVICE) \
               $(LUFA_PATH)/Drivers/USB/Class/Device/CDCClassDevice.c
CC_FLAGS     = -I../src -DUSE_LUFA_CONFIG_HEADER
CPP_FLAGS    = 
CPP_STANDARD = c++11
LD_FLAGS     =
OBJDIR       = _build

# Default target

lib:

include $(LUFA_PATH)/Build/lufa_core.mk
include $(LUFA_PATH)/Build/lufa_sources.mk
include $(LUFA_PATH)/Build/lufa_build.mk
include $(LUFA_PATH)/Build/lufa_cppcheck.mk

