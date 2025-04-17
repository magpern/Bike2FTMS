BOARD ?= pca10056
PROJECT_NAME     := ble_ant_app_FTMS_$(BOARD)_s340
TARGETS          := nrf52840_xxaa_$(BOARD)
OUTPUT_DIRECTORY := _build_$(BOARD)

ifeq ($(BOARD),promicro)
  BOARD_UPPER := PROMICRO
endif
ifeq ($(BOARD),pca10056)
  BOARD_UPPER := PCA10056
endif
ifeq ($(BOARD),xiaoble)
  BOARD_UPPER := XIAOBLE
endif

SDK_ROOT := $(NRF5_SDK_ROOT)
PROJ_DIR := $(abspath ../../../)

$(OUTPUT_DIRECTORY)/nrf52840_xxaa_$(BOARD).out: \
  LINKER_SCRIPT  := SatsBike_gcc_nrf52.ld

# Source files common to all targets
SRC_FILES += \
  $(SDK_ROOT)/modules/nrfx/mdk/gcc_startup_nrf52840.S \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_backend_rtt.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_backend_serial.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_backend_uart.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_default_backends.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_frontend.c \
  $(SDK_ROOT)/components/libraries/log/src/nrf_log_str_formatter.c \
  $(PROJ_DIR)/boards/boards.c \
  $(SDK_ROOT)/modules/nrfx/mdk/system_nrf52840.c \
  $(SDK_ROOT)/components/libraries/button/app_button.c \
  $(SDK_ROOT)/components/libraries/util/app_error.c \
  $(SDK_ROOT)/components/libraries/util/app_error_handler_gcc.c \
  $(SDK_ROOT)/components/libraries/util/app_error_weak.c \
  $(SDK_ROOT)/components/libraries/scheduler/app_scheduler.c \
  $(SDK_ROOT)/components/libraries/timer/app_timer2.c \
  $(SDK_ROOT)/components/libraries/util/app_util_platform.c \
  $(SDK_ROOT)/components/libraries/crc16/crc16.c \
  $(SDK_ROOT)/components/libraries/timer/drv_rtc.c \
  $(SDK_ROOT)/components/libraries/fds/fds.c \
  $(SDK_ROOT)/components/libraries/util/nrf_assert.c \
  $(SDK_ROOT)/components/libraries/atomic_fifo/nrf_atfifo.c \
  $(SDK_ROOT)/components/libraries/atomic_flags/nrf_atflags.c \
  $(SDK_ROOT)/components/libraries/atomic/nrf_atomic.c \
  $(SDK_ROOT)/components/libraries/balloc/nrf_balloc.c \
  $(SDK_ROOT)/external/fprintf/nrf_fprintf.c \
  $(SDK_ROOT)/external/fprintf/nrf_fprintf_format.c \
  $(SDK_ROOT)/components/libraries/fstorage/nrf_fstorage.c \
  $(SDK_ROOT)/components/libraries/fstorage/nrf_fstorage_sd.c \
  $(SDK_ROOT)/components/libraries/memobj/nrf_memobj.c \
  $(SDK_ROOT)/components/libraries/pwr_mgmt/nrf_pwr_mgmt.c \
  $(SDK_ROOT)/components/libraries/ringbuf/nrf_ringbuf.c \
  $(SDK_ROOT)/components/libraries/experimental_section_vars/nrf_section_iter.c \
  $(SDK_ROOT)/components/libraries/sortlist/nrf_sortlist.c \
  $(SDK_ROOT)/components/libraries/strerror/nrf_strerror.c \
  $(SDK_ROOT)/components/libraries/sensorsim/sensorsim.c \
  $(SDK_ROOT)/integration/nrfx/legacy/nrf_drv_uart.c \
  $(SDK_ROOT)/modules/nrfx/soc/nrfx_atomic.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_gpiote.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/prs/nrfx_prs.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_uart.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_uarte.c \
  $(SDK_ROOT)/components/ant/ant_channel_config/ant_channel_config.c \
  $(SDK_ROOT)/components/ant/ant_profiles/ant_bpwr/ant_bpwr.c \
  $(SDK_ROOT)/components/ant/ant_profiles/ant_bpwr/pages/ant_bpwr_page_1.c \
  $(SDK_ROOT)/components/ant/ant_profiles/ant_bpwr/pages/ant_bpwr_page_16.c \
  $(SDK_ROOT)/components/ant/ant_profiles/ant_bpwr/pages/ant_bpwr_page_17.c \
  $(SDK_ROOT)/components/ant/ant_profiles/ant_bpwr/pages/ant_bpwr_page_18.c \
  $(SDK_ROOT)/components/ant/ant_profiles/ant_bpwr/pages/ant_bpwr_page_torque.c \
  $(SDK_ROOT)/components/ant/ant_profiles/ant_common/pages/ant_common_page_80.c \
  $(SDK_ROOT)/components/ant/ant_profiles/ant_common/pages/ant_common_page_81.c \
  $(SDK_ROOT)/components/ant/ant_profiles/ant_bpwr/pages/ant_bpwr_common_data.c \
  $(SDK_ROOT)/components/ant/ant_key_manager/ant_key_manager.c \
  $(SDK_ROOT)/components/libraries/bsp/bsp.c \
  $(PROJ_DIR)/src/main.c \
  $(PROJ_DIR)/src/ble/ble_ftms.c \
  $(PROJ_DIR)/src/ble/ble_cps.c \
  $(PROJ_DIR)/src/utils/device_info.c \
  $(PROJ_DIR)/src/ble/ble_custom_config.c \
  $(PROJ_DIR)/src/nfc/nfc_handler.c \
  $(PROJ_DIR)/src/ble/ble_setup.c \
  $(PROJ_DIR)/src/sensors/reed_sensor.c \
  $(PROJ_DIR)/src/sensors/battery_measurement.c \
  $(PROJ_DIR)/src/ble/ble_ant_scan_service.c \
  $(PROJ_DIR)/src/ble/ble_battery_service.c \
  $(PROJ_DIR)/src/ant/ant_scanner.c \
  $(SDK_ROOT)/external/segger_rtt/SEGGER_RTT.c \
  $(SDK_ROOT)/external/segger_rtt/SEGGER_RTT_Syscalls_GCC.c \
  $(SDK_ROOT)/external/segger_rtt/SEGGER_RTT_printf.c \
  $(SDK_ROOT)/components/ble/peer_manager/auth_status_tracker.c \
  $(SDK_ROOT)/components/ble/common/ble_advdata.c \
  $(SDK_ROOT)/components/ble/ble_advertising/ble_advertising.c \
  $(SDK_ROOT)/components/ble/common/ble_conn_params.c \
  $(SDK_ROOT)/components/ble/common/ble_conn_state.c \
  $(SDK_ROOT)/components/ble/common/ble_srv_common.c \
  $(SDK_ROOT)/components/ble/peer_manager/gatt_cache_manager.c \
  $(SDK_ROOT)/components/ble/peer_manager/gatts_cache_manager.c \
  $(SDK_ROOT)/components/ble/peer_manager/id_manager.c \
  $(SDK_ROOT)/components/ble/nrf_ble_gatt/nrf_ble_gatt.c \
  $(SDK_ROOT)/components/ble/nrf_ble_qwr/nrf_ble_qwr.c \
  $(SDK_ROOT)/components/ble/peer_manager/peer_data_storage.c \
  $(SDK_ROOT)/components/ble/peer_manager/peer_database.c \
  $(SDK_ROOT)/components/ble/peer_manager/peer_id.c \
  $(SDK_ROOT)/components/ble/peer_manager/peer_manager.c \
  $(SDK_ROOT)/components/ble/peer_manager/peer_manager_handler.c \
  $(SDK_ROOT)/components/ble/peer_manager/pm_buffer.c \
  $(SDK_ROOT)/components/ble/peer_manager/security_dispatcher.c \
  $(SDK_ROOT)/components/ble/peer_manager/security_manager.c \
  $(SDK_ROOT)/components/ble/ble_services/ble_bas/ble_bas.c \
  $(SDK_ROOT)/components/ble/ble_services/ble_dis/ble_dis.c \
  $(SDK_ROOT)/components/ble/ble_services/ble_hrs/ble_hrs.c \
  $(SDK_ROOT)/components/softdevice/common/nrf_sdh.c \
  $(SDK_ROOT)/components/softdevice/common/nrf_sdh_ant.c \
  $(SDK_ROOT)/components/softdevice/common/nrf_sdh_ble.c \
  $(SDK_ROOT)/components/softdevice/common/nrf_sdh_soc.c \
  $(SDK_ROOT)/modules/nrfx/hal/nrf_nvmc.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_nfct.c \
  $(SDK_ROOT)/components/nfc/platform/nfc_platform.c \
  $(SDK_ROOT)/integration/nrfx/legacy/nrf_drv_clock.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_clock.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_timer.c \
  $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_saadc.c \

# Include folders common to all targets
INC_FOLDERS += \
  $(SDK_ROOT)/components \
  $(SDK_ROOT)/modules/nrfx/mdk \
  $(SDK_ROOT)/components/libraries/scheduler \
  $(SDK_ROOT)/modules/nrfx \
  $(SDK_ROOT)/components/toolchain/cmsis/include \
  $(SDK_ROOT)/components/libraries/pwr_mgmt \
  $(SDK_ROOT)/components/libraries/sortlist \
  $(SDK_ROOT)/components/libraries/strerror \
  $(SDK_ROOT)/components/softdevice/common \
  $(SDK_ROOT)/components/libraries/crc16 \
  $(SDK_ROOT)/components/libraries/timer \
  $(SDK_ROOT)/components/softdevice/s340/headers \
  $(SDK_ROOT)/components/libraries/util \
  $(SDK_ROOT)/components/softdevice/s340/headers/nrf52 \
  $(SDK_ROOT)/components/ble/common \
  $(SDK_ROOT)/components/libraries/sensorsim \
  $(SDK_ROOT)/components/libraries/balloc \
  $(SDK_ROOT)/components/ble/peer_manager \
  $(SDK_ROOT)/components/libraries/ringbuf \
  $(SDK_ROOT)/modules/nrfx/hal \
  $(SDK_ROOT)/components/libraries/bsp \
  $(SDK_ROOT)/components/ble/ble_services/ble_dis \
  $(SDK_ROOT)/components/libraries/log \
  $(SDK_ROOT)/components/ant/ant_channel_config \
  $(SDK_ROOT)/components/ble/nrf_ble_gatt \
  $(SDK_ROOT)/components/ble/nrf_ble_qwr \
  $(SDK_ROOT)/components/libraries/button \
  $(SDK_ROOT)/components/libraries/fstorage \
  $(SDK_ROOT)/components/libraries/experimental_section_vars \
  $(SDK_ROOT)/integration/nrfx/legacy \
  $(SDK_ROOT)/components/libraries/mutex \
  $(SDK_ROOT)/components/libraries/delay \
  $(SDK_ROOT)/external/segger_rtt \
  $(SDK_ROOT)/components/libraries/atomic_fifo \
  ../config \
  $(SDK_ROOT)/components/libraries/atomic \
  $(SDK_ROOT)/components/libraries/memobj \
  $(SDK_ROOT)/components/libraries/atomic_flags \
  $(SDK_ROOT)/components/ant/ant_key_manager \
  $(SDK_ROOT)/components/ant/ant_key_manager/config \
  $(SDK_ROOT)/integration/nrfx \
  $(SDK_ROOT)/components/libraries/fds \
  $(SDK_ROOT)/components/ble/ble_advertising \
  $(SDK_ROOT)/modules/nrfx/drivers/include \
  $(SDK_ROOT)/components/ble/ble_services/ble_bas \
  $(SDK_ROOT)/external/fprintf \
  $(SDK_ROOT)/components/ble/ble_services/ble_hrs \
  $(SDK_ROOT)/components/libraries/log/src \
  $(SDK_ROOT)/components/ant/ant_profiles/ant_bpwr \
  $(SDK_ROOT)/components/ant/ant_profiles/ant_bpwr/pages \
  $(SDK_ROOT)/components/ant/ant_profiles/ant_bpwr/utils \
  $(SDK_ROOT)/components/ant/ant_profiles/ant_common/pages \
  $(SDK_ROOT)/components/nfc \
  $(SDK_ROOT)/components/nfc/t2t_lib \
  $(SDK_ROOT)/components/nfc/t2t_parser \
  $(SDK_ROOT)/components/nfc/t4t_lib \
  $(SDK_ROOT)/components/nfc/t4t_parser \
  $(SDK_ROOT)/components/nfc/ndef \
  $(SDK_ROOT)/components/nfc/platform \
  $(SDK_ROOT)/modules/nrfx/hal \
  $(SDK_ROOT)/components/libraries/bootloader \
  $(SDK_ROOT)/components/libraries/bootloader/dfu \
  $(SDK_ROOT)/components/ble/ble_services/ble_dfu \
  $(SDK_ROOT)/components/libraries/bootloader/ble_dfu \
  $(SDK_ROOT)/components/libraries/svc \
  $(PROJ_DIR) \
  $(PROJ_DIR)/src \
  $(PROJ_DIR)/src/ble \
  $(PROJ_DIR)/src/ant \
  $(PROJ_DIR)/src/sensors \
  $(PROJ_DIR)/src/utils \
  $(PROJ_DIR)/src/nfc \
  $(PROJ_DIR)/src/includes \
  $(PROJ_DIR)/boards \


# Libraries common to all targets
LIB_FILES += \

ifeq ($(RELEASE),1)
  OPT = -O3 -flto
else
  OPT = -Og -g3
  CFLAGS += -DDEBUG
endif

# C flags common to all targets
CFLAGS += $(OPT)
CFLAGS += -DAPP_TIMER_V2
CFLAGS += -DAPP_TIMER_V2_RTC1_ENABLED
CFLAGS += -DBOARD_$(BOARD_UPPER)
CFLAGS += -DCONFIG_GPIO_AS_PINRESET
CFLAGS += -DFLOAT_ABI_HARD
CFLAGS += -DNRF52840_XXAA
CFLAGS += -DNRF_SD_BLE_API_VERSION=7
CFLAGS += -DS340
CFLAGS += -DSOFTDEVICE_PRESENT
CFLAGS += -mcpu=cortex-m4
CFLAGS += -mthumb -mabi=aapcs
CFLAGS += -Wall -Wno-error
CFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
# keep every function in a separate section, this allows linker to discard unused ones
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
CFLAGS += -fno-builtin -fshort-enums
CFLAGS += -Wno-array-bounds
GIT_VERSION := $(shell git describe --tags --always --dirty)
# Add "-snapshot" if not in CI environment
ifeq ($(CI),1)
  FIRMWARE_VERSION := $(GIT_VERSION)
else
  FIRMWARE_VERSION := $(GIT_VERSION)-snapshot
endif

# Allow manual override via VERSION=...
FIRMWARE_VERSION ?= $(GIT_VERSION)

# Inject version as macro
CFLAGS += -DFIRMWARE_VERSION=\"$(FIRMWARE_VERSION)\"
CFLAGS += -DHARDWARE_REVISION_DEFAULT=\"Rev_$(BOARD_UPPER)\"

ifeq ($(RELEASE),1)
  OPT = -O3
else
  OPT = -Og -g3
  CFLAGS += -DDEBUG
endif

# C++ flags common to all targets
CXXFLAGS += $(OPT)
# Assembler flags common to all targets
ASMFLAGS += -g3
ASMFLAGS += -mcpu=cortex-m4
ASMFLAGS += -mthumb -mabi=aapcs
ASMFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
ASMFLAGS += -DAPP_TIMER_V2
ASMFLAGS += -DAPP_TIMER_V2_RTC1_ENABLED
ASMFLAGS += -DBOARD_$(BOARD_UPPER)
ASMFLAGS += -DCONFIG_GPIO_AS_PINRESET
ASMFLAGS += -DFLOAT_ABI_HARD
ASMFLAGS += -DNRF52840_XXAA
ASMFLAGS += -DNRF_SD_BLE_API_VERSION=7
ASMFLAGS += -DS340
ASMFLAGS += -DSOFTDEVICE_PRESENT

# Linker flags
LDFLAGS += $(OPT)
LDFLAGS += -mthumb -mabi=aapcs -L$(SDK_ROOT)/modules/nrfx/mdk -T$(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m4
LDFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
# let linker dump unused sections
LDFLAGS += -Wl,--gc-sections
# use newlib in nano version
LDFLAGS += --specs=nano.specs
LDFLAGS += -L$(SDK_ROOT)/components/nfc/t2t_lib

CFLAGS += -D__HEAP_SIZE=8192
CFLAGS += -D__STACK_SIZE=8192
ASMFLAGS += -D__HEAP_SIZE=8192
ASMFLAGS += -D__STACK_SIZE=8192

# Add standard libraries at the very end of the linker input, after all objects
# that may need symbols provided by these libraries.
LIB_FILES += -lc -lnosys -lm
LIB_FILES += -l:nfc_t2t_lib_gcc.a


.PHONY: default help

# Default target - first one defined
default: nrf52840_xxaa_$(BOARD)

# Print all targets that can be built
help:
	@echo following targets are available:
	@echo		nrf52840_xxaa
	@echo		sdk_config - starting external tool for editing sdk_config.h
	@echo		flash      - flashing binary

TEMPLATE_PATH := $(SDK_ROOT)/components/toolchain/gcc


include $(PROJ_DIR)/Makefile.common

$(foreach target, $(TARGETS), $(call define_target, $(target)))

.PHONY: flash erase

# Flash the program
flash: default
	@echo Flashing: $(OUTPUT_DIRECTORY)/nrf52840_xxaa_$(BOARD).hex
	nrfjprog -f nrf52 --program $(OUTPUT_DIRECTORY)/nrf52840_xxaa_$(BOARD).hex --sectorerase
	nrfjprog -f nrf52 --reset

.PHONY: build_all build_bootloader

build_bootloader:
	@echo Building bootloader...
	$(MAKE) -C ../../../bootloader/bootloader_s340/armgcc

build_all: build_bootloader default

# Flash softdevice
flash_softdevice:
	@echo Flashing: s340_nrf52_7.0.1.hex
	nrfjprog -f nrf52 --program $(SDK_ROOT)/components/softdevice/s340/hex/ANT_s340_nrf52_7.0.1.hex --sectorerase
	nrfjprog -f nrf52 --reset

SOFTDEVICE_HEX := $(SDK_ROOT)/components/softdevice/s340/hex/ANT_s340_nrf52_7.0.1.hex
BOOTLOADER_HEX := ../../../bootloader/bootloader_s340/armgcc/_build_$(BOARD)/bikeble_bootloader_$(BOARD).hex
APP_HEX        := $(OUTPUT_DIRECTORY)/nrf52840_xxaa_$(BOARD).hex
MERGED_HEX     := merged_firmware_$(BOARD).hex
DFU_SETTINGS   := dfu_settings.hex

#change this VALIDATE_GENERATED_CRC
merge_all:
	@echo Generating DFU settings...
	nrfutil settings generate --family NRF52840 \
	  --application $(APP_HEX) \
	  --application-version 1 \
	  --bootloader-version 1 \
	  --bl-settings-version 2 \
    --app-boot-validation VALIDATE_GENERATED_CRC \
	  $(DFU_SETTINGS)

	@echo Merging: app + softdevice + bootloader + dfu_settings
	mergehex --merge \
	  $(SOFTDEVICE_HEX) \
	  $(BOOTLOADER_HEX) \
	  $(APP_HEX) \
	  $(DFU_SETTINGS) \
	  --output $(MERGED_HEX)

	@echo Done! $(MERGED_HEX)

PKG_NAME       := bikeble_$(BOARD)_dfu.zip
HW_VERSION    := 52
APP_VERSION   := 1
SD_ID         := 0xCE  # Use actual value if SoftDevice has a known ID
SD_REQ        := 0xCE

pkg: merge_all
	@echo Creating DFU package...
	nrfutil pkg generate --hw-version $(HW_VERSION) \
	  --application $(APP_HEX) \
	  --application-version $(APP_VERSION) \
	  --softdevice $(SOFTDEVICE_HEX) \
	  --sd-id $(SD_ID) \
	  --sd-req $(SD_REQ) \
    --key-file private_key.pem \
	  $(PKG_NAME)
	@echo Done! $(PKG_NAME)

flash_all: build_all merge_all
	@echo Flashing merged firmware...
	nrfjprog -f nrf52 --program $(MERGED_HEX) --sectorerase
	nrfjprog -f nrf52 --reset
	@echo Flash complete!
  
erase:
	nrfjprog -f nrf52 --eraseall

APP_HEX        := $(OUTPUT_DIRECTORY)/nrf52840_xxaa_$(BOARD).hex
DFU_SETTINGS   := dfu_settings.hex
MERGED_APP_HEX := merged_app_$(BOARD).hex

flashapp: default
	@echo "⚙️  Generating DFU settings with CRC..."
	nrfutil settings generate --family NRF52840 \
	  --application $(APP_HEX) \
	  --application-version 1 \
	  --bootloader-version 1 \
	  --bl-settings-version 2 \
	  --app-boot-validation VALIDATE_GENERATED_CRC \
	  $(DFU_SETTINGS)

	@echo "🔀 Merging application + DFU settings..."
	mergehex --merge $(APP_HEX) $(DFU_SETTINGS) --output $(MERGED_APP_HEX)

	@echo "⚡ Flashing application only..."
	nrfjprog -f nrf52 --program $(MERGED_APP_HEX) --sectorerase
	nrfjprog -f nrf52 --reset
	@echo "✅ Done: Application with DFU settings flashed."

SDK_CONFIG_FILE := ../config/sdk_config.h
CMSIS_CONFIG_TOOL := $(SDK_ROOT)/external_tools/cmsisconfig/CMSIS_Configuration_Wizard.jar
sdk_config:
	java -jar $(CMSIS_CONFIG_TOOL) $(SDK_CONFIG_FILE)

BOARDS := promicro pca10056 xiaoble
BOOTLOADER_DIR := ../../../bootloader/bootloader_s340/armgcc

.PHONY: package_all
package_all:
	$(MAKE) -C $(BOOTLOADER_DIR) BOARD=promicro RELEASE=1
	$(MAKE) BOARD=promicro RELEASE=1 default pkg
	$(MAKE) -C $(BOOTLOADER_DIR) BOARD=pca10056 RELEASE=1
	$(MAKE) BOARD=pca10056 RELEASE=1 default pkg
	$(MAKE) -C $(BOOTLOADER_DIR) BOARD=xiaoble RELEASE=1
	$(MAKE) BOARD=xiaoble RELEASE=1 default pkg




