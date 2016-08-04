# This file is part of Espruino, a JavaScript interpreter for Microcontrollers
#
# Copyright (C) 2013 Gordon Williams <gw@pur3.co.uk>
# Copyright (C) 2014 Alain Sézille for NucleoF401RE, NucleoF411RE specific lines of this file
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#


# super hacky makefile for just NFC
GENDIR=$(shell pwd)/gen

NFC=1
NRF52832DK=1

INCLUDE=-I$(ROOT) -I$(ROOT)/targets -I$(ROOT)/src -I$(GENDIR)
LIBS=
DEFINES=
CFLAGS=-Wall -Wextra -Wconversion -Werror=implicit-function-declaration -fno-strict-aliasing
LDFLAGS=-Winline
OPTIMIZEFLAGS=
#-fdiagnostics-show-option - shows which flags can be used with -Werror
DEFINES+=-DGIT_COMMIT=$(shell git log -1 --format="%H")

# Espruino flags...
USE_MATH=1

STAT_FLAGS='-c ''%s'''
ifdef RELEASE
# force no asserts to be compiled in
DEFINES += -DNO_ASSERT -DRELEASE
endif

# Default release labeling.  (This may fail and give inconsistent results due to the fact that
# travis does a shallow clone.)
LATEST_RELEASE=$(shell git tag | grep RELEASE_ | sort | tail -1)
# use egrep to count lines instead of wc to avoid whitespace error on Mac
COMMITS_SINCE_RELEASE=$(shell git log --oneline $(LATEST_RELEASE)..HEAD | egrep -c .)
ifneq ($(COMMITS_SINCE_RELEASE),0)
DEFINES += -DBUILDNUMBER=\"$(COMMITS_SINCE_RELEASE)\"
endif

CWD = $(CURDIR)
ROOT = $(CWD)
PRECOMPILED_OBJS=
BASEADDRESS=0x08000000

EMBEDDED=1
BOARD=NRF52832DK
OPTIMIZEFLAGS+=-O3
USE_BLUETOOTH=1
DEFINES += -DBOARD_PCA10040 -DPCA10040
# ----------------------------- end of board defines ------------------------------
# ---------------------------------------------------------------------------------


# ---------------------------------------------------------------------------------
#                                                      Get info out of BOARDNAME.py
# ---------------------------------------------------------------------------------

PROJ_NAME=$(shell python scripts/get_board_info.py $(BOARD) "common.get_board_binary_name(board)"  | sed -e "s/.bin$$//")
ifeq ($(PROJ_NAME),)
$(error Unable to work out binary name (PROJ_NAME))
endif
ifeq ($(BOARD),LINUX)
PROJ_NAME=espruino
endif

ifeq ($(shell python scripts/get_board_info.py $(BOARD) "'bootloader' in board.info and board.info['bootloader']==1"),True)
USE_BOOTLOADER:=1
BOOTLOADER_PROJ_NAME:=bootloader_$(PROJ_NAME)
DEFINES+=-DUSE_BOOTLOADER
endif

ifeq ($(shell python scripts/get_board_info.py $(BOARD) "'USB' in board.devices"),True)
USB:=1
endif

ifndef LINUX
FAMILY:=$(shell python scripts/get_board_info.py $(BOARD) "board.chip['family']")
CHIP:=$(shell python scripts/get_board_info.py $(BOARD) "board.chip['part']")
endif

# ---------------------------------------------------------------------------------


ifdef DEBUG
#OPTIMIZEFLAGS=-Os -g
 ifeq ($(FAMILY),ESP8266)
  OPTIMIZEFLAGS=-g -Os -std=gnu11 -fgnu89-inline -Wl,--allow-multiple-definition
 else
  OPTIMIZEFLAGS=-g
 endif
 ifdef EFM32
  DEFINES += -DDEBUG_EFM=1 -DDEBUG=1
 endif
DEFINES+=-DDEBUG
endif

ifdef PROFILE
OPTIMIZEFLAGS+=-pg
endif

# These are files for platform-specific libraries
TARGETSOURCES =
WRAPPERSOURCES = 
SOURCES =
CPPSOURCES =


ifeq ($(FAMILY), NRF51)

  NRF5X=1
  NRF5X_SDK_PATH=$(ROOT)/targetlibs/nrf5x/nrf5_sdk

  # ARCHFLAGS are shared by both CFLAGS and LDFLAGS.
  ARCHFLAGS = -mcpu=cortex-m0 -mthumb -mabi=aapcs -mfloat-abi=soft # Use nRF51 makefiles provided in SDK as reference.

  # nRF51 specific.
  INCLUDE          += -I$(NRF5X_SDK_PATH)/../nrf51_config
  INCLUDE          += -I$(NRF5X_SDK_PATH)/components/softdevice/s130/headers
  INCLUDE          += -I$(NRF5X_SDK_PATH)/components/softdevice/s130/headers/nrf51
  TARGETSOURCES    += $(NRF5X_SDK_PATH)/components/toolchain/system_nrf51.c
  PRECOMPILED_OBJS += $(NRF5X_SDK_PATH)/components/toolchain/gcc/gcc_startup_nrf51.o

  DEFINES += -DNRF51 -DSWI_DISABLE0 -DSOFTDEVICE_PRESENT -DS130 -DBLE_STACK_SUPPORT_REQD -DNRF_LOG_USES_UART # SoftDevice included by default.
  LINKER_RAM:=$(shell python scripts/get_board_info.py $(BOARD) "board.chip['ram']")

  SOFTDEVICE        = $(NRF5X_SDK_PATH)/components/softdevice/s130/hex/s130_nrf51_2.0.0_softdevice.hex

  LINKER_FILE = $(NRF5X_SDK_PATH)/../nrf5x_linkers/linker_nrf51_ble_espruino_$(LINKER_RAM).ld

  ifdef USE_BOOTLOADER
  NRF_BOOTLOADER    = $(ROOT)/targetlibs/nrf5x/nrf5_singlebank_bl_hex/nrf51_s130_singlebank_bl.hex
  NFR_BL_START_ADDR = 0x3C000
  NRF_BOOTLOADER_SETTINGS = $(ROOT)/targetlibs/nrf5x/nrf5_singlebank_bl_hex/bootloader_settings_nrf51.hex # This file writes 0x3FC00 with 0x01 so we can flash the application with the bootloader.

  endif

endif # FAMILY == NRF51

ifeq ($(FAMILY), NRF52)

  NRF5X=1
  NRF5X_SDK_PATH=$(ROOT)/targetlibs/nrf5x/nrf5_sdk

  # ARCHFLAGS are shared by both CFLAGS and LDFLAGS.
  ARCHFLAGS = -mcpu=cortex-m4 -mthumb -mabi=aapcs -mfloat-abi=hard -mfpu=fpv4-sp-d16

  # nRF52 specific.
  INCLUDE          += -I$(NRF5X_SDK_PATH)/../nrf52_config
  INCLUDE          += -I$(NRF5X_SDK_PATH)/components/softdevice/s132/headers
  INCLUDE          += -I$(NRF5X_SDK_PATH)/components/softdevice/s132/headers/nrf52
  TARGETSOURCES    += $(NRF5X_SDK_PATH)/components/toolchain/system_nrf52.c \
                      $(NRF5X_SDK_PATH)/components/drivers_nrf/hal/nrf_saadc.c
  PRECOMPILED_OBJS += $(NRF5X_SDK_PATH)/components/toolchain/gcc/gcc_startup_nrf52.o

  DEFINES += -DSWI_DISABLE0 -DSOFTDEVICE_PRESENT -DNRF52 -DCONFIG_GPIO_AS_PINRESET -DS132 -DBLE_STACK_SUPPORT_REQD -DNRF_LOG_USES_UART

  SOFTDEVICE        = $(NRF5X_SDK_PATH)/components/softdevice/s132/hex/s132_nrf52_2.0.0_softdevice.hex

  LINKER_FILE = $(NRF5X_SDK_PATH)/../nrf5x_linkers/linker_nrf52_ble_espruino.ld # TODO: Should have separate linkers like is done in nrf_bootloader branch.

  ifdef USE_BOOTLOADER
  NRF_BOOTLOADER    = $(ROOT)/targetlibs/nrf5x/nrf5_singlebank_bl_hex/nrf52_s132_singlebank_bl.hex
  NFR_BL_START_ADDR = 0x7A000
  NRF_BOOTLOADER_SETTINGS = $(ROOT)/targetlibs/nrf5x/nrf5_singlebank_bl_hex/bootloader_settings_nrf52.hex # Writes address 0x7F000 with 0x01.
  endif
endif #FAMILY == NRF52


ifdef NFC
  DEFINES += -DUSE_NFC
  INCLUDE          += -I$(NRF5X_SDK_PATH)/components/drivers_nrf/clock
  INCLUDE          += -I$(NRF5X_SDK_PATH)/components/nfc/t2t_lib
  INCLUDE          += -I$(NRF5X_SDK_PATH)/components/nfc/ndef/uri
  INCLUDE          += -I$(NRF5X_SDK_PATH)/components/nfc/ndef/generic/message
  INCLUDE          += -I$(NRF5X_SDK_PATH)/components/nfc/ndef/generic/record
  TARGETSOURCES    += $(NRF5X_SDK_PATH)/components/libraries/util/nrf_log.c
  TARGETSOURCES    += $(NRF5X_SDK_PATH)/components/libraries/util/app_util_platform.c
  TARGETSOURCES    += $(NRF5X_SDK_PATH)/components/drivers_nrf/clock/nrf_drv_clock.c
  TARGETSOURCES    += $(NRF5X_SDK_PATH)/components/nfc/ndef/uri/nfc_uri_msg.c
  TARGETSOURCES    += $(NRF5X_SDK_PATH)/components/nfc/ndef/uri/nfc_uri_rec.c
  TARGETSOURCES    += $(NRF5X_SDK_PATH)/components/nfc/ndef/generic/message/nfc_ndef_msg.c
  TARGETSOURCES    += $(NRF5X_SDK_PATH)/components/nfc/ndef/generic/record/nfc_ndef_record.c
  TARGETSOURCES    += $(NRF5X_SDK_PATH)/components/nfc/t2t_lib/hal_t2t/hal_nfc_t2t.c
  PRECOMPILED_OBJS += $(NRF5X_SDK_PATH)/components/nfc/t2t_lib/nfc_t2t_lib_gcc.a
endif
ifdef NRF5X

  # Just try and get rid of the compile warnings.
  CFLAGS += -Wno-sign-conversion -Wno-conversion -Wno-unused-parameter -fomit-frame-pointer #this is for device manager in nordic sdk
  DEFINES += -DBLUETOOTH -D$(BOARD)

  ARM = 1
  ARM_HAS_OWN_CMSIS = 1 # Nordic uses its own CMSIS files in its SDK, these are up-to-date.
  INCLUDE += -I$(ROOT)/targetlibs/nrf5x -I$(NRF5X_SDK_PATH)

  TEMPLATE_PATH = $(ROOT)/targetlibs/nrf5x/nrf5x_linkers # This is where the common linker for both nRF51 & nRF52 is stored.
  LDFLAGS += -L$(TEMPLATE_PATH)

  # These files are the Espruino HAL implementation.
  INCLUDE += -I$(ROOT)/targets/nrf5x
  SOURCES +=                              \
  targets/nrf5x/main.c

  # Careful here.. All these includes and sources assume a SoftDevice. Not efficeint/clean if softdevice (ble) is not enabled...
  INCLUDE += -I$(NRF5X_SDK_PATH)/components
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/drivers_nrf/config
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/libraries/fstorage/config
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/libraries/util
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/drivers_nrf/delay
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/drivers_nrf/uart
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/ble/common
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/drivers_nrf/pstorage
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/libraries/uart
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/device
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/libraries/button
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/libraries/timer
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/libraries/fstorage
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/libraries/experimental_section_vars
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/drivers_nrf/gpiote
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/ble/ble_services/ble_nus
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/toolchain/CMSIS/Include
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/drivers_nrf/hal
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/toolchain/gcc
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/toolchain
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/drivers_nrf/common
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/ble/ble_advertising
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/libraries/trace
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/softdevice/common/softdevice_handler
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/drivers_nrf/twi_master
	INCLUDE += -I$(NRF5X_SDK_PATH)/components/drivers_nrf/hal/nrf_pwm

  TARGETSOURCES += \
  $(NRF5X_SDK_PATH)/components/libraries/util/app_error.c \
  $(NRF5X_SDK_PATH)/components/libraries/timer/app_timer.c \
  $(NRF5X_SDK_PATH)/components/libraries/fstorage/fstorage.c \
  $(NRF5X_SDK_PATH)/components/libraries/trace/app_trace.c \
  $(NRF5X_SDK_PATH)/components/libraries/util/nrf_assert.c \
  $(NRF5X_SDK_PATH)/components/libraries/uart/app_uart.c \
  $(NRF5X_SDK_PATH)/components/drivers_nrf/delay/nrf_delay.c \
  $(NRF5X_SDK_PATH)/components/drivers_nrf/common/nrf_drv_common.c \
  $(NRF5X_SDK_PATH)/components/drivers_nrf/gpiote/nrf_drv_gpiote.c \
  $(NRF5X_SDK_PATH)/components/drivers_nrf/uart/nrf_drv_uart.c \
  $(NRF5X_SDK_PATH)/components/drivers_nrf/pstorage/pstorage.c \
  $(NRF5X_SDK_PATH)/components/ble/common/ble_advdata.c \
  $(NRF5X_SDK_PATH)/components/ble/ble_advertising/ble_advertising.c \
  $(NRF5X_SDK_PATH)/components/ble/common/ble_conn_params.c \
  $(NRF5X_SDK_PATH)/components/ble/ble_services/ble_nus/ble_nus.c \
  $(NRF5X_SDK_PATH)/components/ble/common/ble_srv_common.c \
  $(NRF5X_SDK_PATH)/components/softdevice/common/softdevice_handler/softdevice_handler.c \
  $(NRF5X_SDK_PATH)/components/drivers_nrf/hal/nrf_nvmc.c \
  $(NRF5X_SDK_PATH)/components/drivers_nrf/twi_master/nrf_drv_twi.c \
  $(NRF5X_SDK_PATH)/components/drivers_nrf/hal/nrf_adc.c
  # $(NRF5X_SDK_PATH)/components/libraries/util/nrf_log.c

  ifdef USE_BOOTLOADER
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/ble/device_manager
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/ble/ble_services/ble_dfu
  INCLUDE += -I$(NRF5X_SDK_PATH)/components/libraries/bootloader_dfu
  TARGETSOURCES += \
   $(NRF5X_SDK_PATH)/components/ble/device_manager/device_manager_peripheral.c \
   $(NRF5X_SDK_PATH)/components/ble/ble_services/ble_dfu/ble_dfu.c \
   $(NRF5X_SDK_PATH)/components/libraries/bootloader_dfu/bootloader_util.c \
   $(NRF5X_SDK_PATH)/components/libraries/bootloader_dfu/dfu_app_handler.c
  endif

endif #NRF5X

ifdef ARM

  ifndef LINKER_FILE # nRF5x targets define their own linker file.
    LINKER_FILE = $(GENDIR)/linker.ld
  endif
  DEFINES += -DARM
  ifndef ARM_HAS_OWN_CMSIS # nRF5x targets do not use the shared CMSIS files.
    INCLUDE += -I$(ROOT)/targetlibs/arm
  endif
  OPTIMIZEFLAGS += -fno-common -fno-exceptions -fdata-sections -ffunction-sections

  # I've no idea why this breaks the bootloader, but it does.
  # Given we've left 10k for it, there's no real reason to enable LTO anyway.
  ifndef BOOTLOADER
	# Enable link-time optimisations (inlining across files)
	OPTIMIZEFLAGS += -flto -fno-fat-lto-objects -Wl,--allow-multiple-definition
	DEFINES += -DLINK_TIME_OPTIMISATION
  endif

  export CCPREFIX?=arm-none-eabi-

endif # ARM

SOURCES += $(WRAPPERSOURCES) $(TARGETSOURCES)
SOURCEOBJS = $(SOURCES:.c=.o) $(CPPSOURCES:.cpp=.o)
OBJS = $(SOURCEOBJS) $(PRECOMPILED_OBJS)


# -ffreestanding -nodefaultlibs -nostdlib -fno-common
# -nodefaultlibs -nostdlib -nostartfiles

# -fdata-sections -ffunction-sections are to help remove unused code
CFLAGS += $(OPTIMIZEFLAGS) -c $(ARCHFLAGS) $(DEFINES) $(INCLUDE)

# -Wl,--gc-sections helps remove unused code
# -Wl,--whole-archive checks for duplicates
ifdef NRF5X
 LDFLAGS += $(ARCHFLAGS)
 LDFLAGS += --specs=nano.specs -lc -lnosys
else ifdef EFM32
 LDFLAGS += $(OPTIMIZEFLAGS) $(ARCHFLAGS)
 LDFLAGS += -Wl,--start-group -lgcc -lc -lnosys -Wl,--end-group
else
 LDFLAGS += $(OPTIMIZEFLAGS) $(ARCHFLAGS)
endif

ifdef EMBEDDED
DEFINES += -DEMBEDDED
LDFLAGS += -Wl,--gc-sections
endif

ifdef LINKER_FILE
  LDFLAGS += -T$(LINKER_FILE)
endif

export CC=$(CCPREFIX)gcc
export LD=$(CCPREFIX)gcc
export AR=$(CCPREFIX)ar
export AS=$(CCPREFIX)as
export OBJCOPY=$(CCPREFIX)objcopy
export OBJDUMP=$(CCPREFIX)objdump
export GDB=$(CCPREFIX)gdb


.PHONY:  proj

all: 	 proj

ifeq ($(V),1)
        quiet_=
        Q=
else
        quiet_=quiet_
        Q=@
  export SILENT=1
endif

compile=$(CC) $(CFLAGS) $< -o $@
link=$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
# note: link is ignored for the ESP8266
obj_dump=$(OBJDUMP) -x -S $(PROJ_NAME).elf > $(PROJ_NAME).lst
obj_to_bin=$(OBJCOPY) -O $1 $(PROJ_NAME).elf $(PROJ_NAME).$2

quiet_compile= CC $@
quiet_link= LD $@
quiet_obj_dump= GEN $(PROJ_NAME).lst
quiet_obj_to_bin= GEN $(PROJ_NAME).$2

%.o: %.c 
	@echo $($(quiet_)compile)
	@$(call compile)

.s.o:
	@echo $($(quiet_)compile)
	@$(call compile)

$(PROJ_NAME).elf: $(OBJS) $(LINKER_FILE)
	@echo $($(quiet_)link)
	@$(call link)

$(PROJ_NAME).lst : $(PROJ_NAME).elf
	@echo $($(quiet_)obj_dump)
	@$(call obj_dump)

$(PROJ_NAME).hex: $(PROJ_NAME).elf
	@echo $(call $(quiet_)obj_to_bin,ihex,hex)
	@$(call obj_to_bin,ihex,hex)
ifdef SOFTDEVICE # Shouldn't do this when we want to be able to perform DFU OTA!
 ifdef USE_BOOTLOADER
  ifdef DFU_UPDATE_BUILD
	echo Not merging softdevice or bootloader with application
  else
	echo Merging SoftDevice and Bootloader
	scripts/hexmerge.py $(SOFTDEVICE) $(NRF_BOOTLOADER):$(NFR_BL_START_ADDR): $(PROJ_NAME).hex $(NRF_BOOTLOADER_SETTINGS) -o tmp.hex
	mv tmp.hex $(PROJ_NAME).hex
  endif
 else
	echo Merging SoftDevice
	scripts/hexmerge.py $(SOFTDEVICE) $(PROJ_NAME).hex -o tmp.hex
	mv tmp.hex $(PROJ_NAME).hex
 endif # USE_BOOTLOADER
endif # SOFTDEVICE

$(PROJ_NAME).srec : $(PROJ_NAME).elf
	@echo $(call $(quiet_)obj_to_bin,srec,srec)
	@$(call obj_to_bin,srec,srec)

$(PROJ_NAME).bin : $(PROJ_NAME).elf
	@echo $(call $(quiet_)obj_to_bin,binary,bin)
	@$(call obj_to_bin,binary,bin)

proj: $(PROJ_NAME).lst $(PROJ_NAME).bin $(PROJ_NAME).hex

#proj: $(PROJ_NAME).lst $(PROJ_NAME).hex $(PROJ_NAME).srec $(PROJ_NAME).bin

flash: all
	if [ -d "/media/$(USER)/JLINK" ]; then cp $(PROJ_NAME).hex /media/$(USER)/JLINK;sync; fi
	if [ -d "/media/JLINK" ]; then cp $(PROJ_NAME).hex /media/JLINK;sync; fi

serialflash: all
	echo STM32 inbuilt serial bootloader, set BOOT0=1, BOOT1=0
	python scripts/stm32loader.py -b 460800 -a $(BASEADDRESS) -ew $(STM32LOADER_FLAGS) $(PROJ_NAME).bin
#	python scripts/stm32loader.py -b 460800 -a $(BASEADDRESS) -ewv $(STM32LOADER_FLAGS) $(PROJ_NAME).bin

gdb:
	echo "target extended-remote :4242" > gdbinit
	echo "file $(PROJ_NAME).elf" >> gdbinit
	#echo "load" >> gdbinit
	echo "break main" >> gdbinit
	echo "break HardFault_Handler" >> gdbinit
	$(GDB) -x gdbinit
	rm gdbinit
	    # ---------------------------------------------------
clean:
	@echo Cleaning targets
	$(Q)find . -name \*.o | grep -v libmbed | grep -v arm-bcm2708 | xargs rm -f
	$(Q)rm -f $(ROOT)/gen/*.c $(ROOT)/gen/*.h $(ROOT)/gen/*.ld
	$(Q)rm -f $(PROJ_NAME).elf
	$(Q)rm -f $(PROJ_NAME).hex
	$(Q)rm -f $(PROJ_NAME).bin
	$(Q)rm -f $(PROJ_NAME).srec
	$(Q)rm -f $(PROJ_NAME).lst

# start make like this "make varsonly" to get all variables created and used during make process without compiling
# this helps to better understand linking, or to find oddities
varsonly:
	$(foreach v, $(.VARIABLES), $(info $(v) = $($(v))))
