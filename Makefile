BOARD_TAG    = teensyLC
ARDUINO_LIBS =
F_CPU        = 48000000
MCU          = cortex-m0plus
USB_TYPE     = USB_SERIAL

ARDUINO_DIR ?= $(ARDUINOPATH)

LOCAL_C_SRCS    ?= $(wildcard ./src/*.c)
LOCAL_CPP_SRCS  ?= $(wildcard ./src/*.cpp)
LOCAL_CC_SRCS   ?= $(wildcard ./src/*.cc)
LOCAL_PDE_SRCS  ?= $(wildcard ./src/*.pde)
LOCAL_INO_SRCS  ?= $(wildcard ./src/*.ino)
LOCAL_AS_SRCS   ?= $(wildcard ./src/*.S)

OPTIONS += -DTEENSYDUINO=129 -D__MKL26Z64__ -DF_CPU=$(F_CPU) -I./include

# CPPFLAGS = compiler options for C and C++
CPPFLAGS = -Wall -g -ffunction-sections -fdata-sections -nostdlib \
    -Os \
    -mthumb -fsingle-precision-constant \
    -MMD $(OPTIONS) -I.

# compiler options for C++ only
CXXFLAGS = -fno-exceptions -felide-constructors -std=gnu++0x -fno-rtti

LDFLAGS = -Wl,--gc-sections,--relax,--defsym=__rtc_localtime=0\
    --specs=nano.specs\
    -mthumb \
    -T$(ARDUINO_CORE_PATH)/mkl26z64.ld

include ./build-system/Teensy.mk
