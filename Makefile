# Kernel build directory
KDIR := /lib/modules/$(shell uname -r)/build
RADIO_TEST := 0
FW_LOAD_SUPPORT := 1
FW_LOAD?=PATCH
# Default is empty to force explicit selection
BUS_IF := PCIE

# Due to multiple Makefiles, we need to get the current directory
NRF_WIFI_DIR := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))

INCLUDES = -I$(NRF_WIFI_DIR)/utils/inc \
		   -I$(NRF_WIFI_DIR)/os_if/inc \
		   -I$(NRF_WIFI_DIR)/bus_if/bal/inc \
		   -I$(NRF_WIFI_DIR)/fw_if/umac_if/inc \
		   -I$(NRF_WIFI_DIR)/fw_load/mips/fw/inc \
		   -I$(NRF_WIFI_DIR)/hw_if/hal/inc \
		   -I$(NRF_WIFI_DIR)/hw_if/hal/inc/fw \
		   -I$(NRF_WIFI_DIR)/fw_if/umac_if/inc/fw

ifeq ($(BUS_IF), QSPI)
	INCLUDES += -I$(NRF_WIFI_DIR)/bus_if/bus/qspi/inc
else ifeq ($(BUS_IF), SPI)
	INCLUDES += -I$(NRF_WIFI_DIR)/bus_if/bus/spi/inc
else ifeq ($(BUS_IF), PCIE)
	INCLUDES += -I$(NRF_WIFI_DIR)/bus_if/bus/pcie/inc
endif

# TODO: Use Kconfig + menuconfig for this
# For now, just comment/uncomment the flags you want
ccflags-y += -DNRF_WIFI_LOW_POWER
ccflags-y += -DNRF_WIFI_RPU_RECOVERY
ccflags-y += -DNRF_WIFI_AP_DEAD_DETECT_TIMEOUT=20
ccflags-y += -DNRF_WIFI_IFACE_MTU=1500
ccflags-y += -DNRF70_STA_MODE
ccflags-y += -DNRF70_DATA_TX
#ccflags-y += -DNRF70_RAW_DATA_TX
#ccflags-y += -DNRF70_RAW_DATA_RX
#ccflags-y += -DNRF70_PROMISC_DATA_RX
#ccflags-y += -DNRF70_TX_DONE_WQ_ENABLED
#ccflags-y += -DNRF70_RX_WQ_ENABLED
ccflags-y += -DNRF70_UTIL
#ccflags-y += -DNRF70_OFFLOADED_RAW_TX
ccflags-y += -DNRF70_TCP_IP_CHECKSUM_OFFLOAD
ccflags-y += -DNRF70_RPU_EXTEND_TWT_SP
#ccflags-y += -DNRF70_SYSTEM_WITH_RAW_MODES
#ccflags-y += -DNRF70_SCAN_ONLY
#ccflags-y += -DNRF70_2_4G_ONLY
ccflags-y += -DNRF70_LOG_VERBOSE
#ccflags-y += -DNRF70_AP_MODE
ccflags-y += -DNRF_WIFI_MGMT_BUFF_OFFLOAD
ccflags-y += -DNRF_WIFI_FEAT_KEEPALIVE
ccflags-y += -DNRF_WIFI_KEEPALIVE_PERIOD_S=60
#ccflags-y += -DWIFI_MGMT_RAW_SCAN_RESULTS
#ccflags-y += -DNRF_WIFI_COEX_DISABLE_PRIORITY_WINDOW_FOR_SCAN
ccflags-y += -DNRF70_RX_NUM_BUFS=48
ccflags-y += -DNRF70_MAX_TX_TOKENS=10
ccflags-y += -DNRF70_RX_MAX_DATA_SIZE=1600
ccflags-y += -DNRF70_MAX_TX_PENDING_QLEN=18
ccflags-y += -DNRF70_RPU_PS_IDLE_TIMEOUT_MS=10
ccflags-y += -DNRF70_BAND_2G_LOWER_EDGE_BACKOFF_DSSS=0
ccflags-y += -DNRF70_BAND_2G_LOWER_EDGE_BACKOFF_HT=0
ccflags-y += -DNRF70_BAND_2G_LOWER_EDGE_BACKOFF_HE=0
ccflags-y += -DNRF70_BAND_2G_UPPER_EDGE_BACKOFF_DSSS=0
ccflags-y += -DNRF70_BAND_2G_UPPER_EDGE_BACKOFF_HT=0
ccflags-y += -DNRF70_BAND_2G_UPPER_EDGE_BACKOFF_HE=0
ccflags-y += -DNRF70_BAND_UNII_1_LOWER_EDGE_BACKOFF_HT=0
ccflags-y += -DNRF70_BAND_UNII_1_LOWER_EDGE_BACKOFF_HE=0
ccflags-y += -DNRF70_BAND_UNII_1_UPPER_EDGE_BACKOFF_HT=0
ccflags-y += -DNRF70_BAND_UNII_1_UPPER_EDGE_BACKOFF_HE=0
ccflags-y += -DNRF70_BAND_UNII_2A_LOWER_EDGE_BACKOFF_HT=0
ccflags-y += -DNRF70_BAND_UNII_2A_LOWER_EDGE_BACKOFF_HE=0
ccflags-y += -DNRF70_BAND_UNII_2A_UPPER_EDGE_BACKOFF_HT=0
ccflags-y += -DNRF70_BAND_UNII_2A_UPPER_EDGE_BACKOFF_HE=0
ccflags-y += -DNRF70_BAND_UNII_2C_LOWER_EDGE_BACKOFF_HT=0
ccflags-y += -DNRF70_BAND_UNII_2C_LOWER_EDGE_BACKOFF_HE=0
ccflags-y += -DNRF70_BAND_UNII_2C_UPPER_EDGE_BACKOFF_HT=0
ccflags-y += -DNRF70_BAND_UNII_2C_UPPER_EDGE_BACKOFF_HE=0
ccflags-y += -DNRF70_BAND_UNII_3_LOWER_EDGE_BACKOFF_HT=0
ccflags-y += -DNRF70_BAND_UNII_3_LOWER_EDGE_BACKOFF_HE=0
ccflags-y += -DNRF70_BAND_UNII_3_UPPER_EDGE_BACKOFF_HT=0
ccflags-y += -DNRF70_BAND_UNII_3_UPPER_EDGE_BACKOFF_HE=0
ccflags-y += -DNRF70_BAND_UNII_4_LOWER_EDGE_BACKOFF_HT=0
ccflags-y += -DNRF70_BAND_UNII_4_LOWER_EDGE_BACKOFF_HE=0
ccflags-y += -DNRF70_BAND_UNII_4_UPPER_EDGE_BACKOFF_HT=0
ccflags-y += -DNRF70_BAND_UNII_4_UPPER_EDGE_BACKOFF_HE=0
ccflags-y += -DNRF70_PCB_LOSS_2G=0
ccflags-y += -DNRF70_PCB_LOSS_5G_BAND1=0
ccflags-y += -DNRF70_PCB_LOSS_5G_BAND2=0
ccflags-y += -DNRF70_PCB_LOSS_5G_BAND3=0
ccflags-y += -DNRF70_ANT_GAIN_2G=0
ccflags-y += -DNRF70_ANT_GAIN_5G_BAND1=0
ccflags-y += -DNRF70_ANT_GAIN_5G_BAND2=0
ccflags-y += -DNRF70_ANT_GAIN_5G_BAND3=0
ccflags-y += -DNRF_WIFI_PS_INT_PS
ccflags-y += -DNRF_WIFI_RPU_RECOVERY_PS_ACTIVE_TIMEOUT_MS=50000
ccflags-y += -DNRF_WIFI_DISPLAY_SCAN_BSS_LIMIT=150
ccflags-y += -DNRF_WIFI_RPU_MIN_TIME_TO_ENTER_SLEEP_MS=1000
ccflags-y += -DWIFI_NRF70_LOG_LEVEL=1

# Source files
SRCS = os_if/src/osal.c \
	   utils/src/list.c \
	   utils/src/queue.c \
	   utils/src/util.c \
	   hw_if/hal/src/hal_api.c \
	   hw_if/hal/src/hal_interrupt.c \
	   hw_if/hal/src/hal_mem.c \
	   hw_if/hal/src/hal_reg.c \
	   hw_if/hal/src/hpqm.c \
	   hw_if/hal/src/pal.c \
	   bus_if/bal/src/bal.c \
	   fw_if/umac_if/src/cmd.c \
	   fw_if/umac_if/src/event.c \
	   fw_if/umac_if/src/fmac_api_common.c \
	   fw_if/umac_if/src/fmac_peer.c \
	   fw_if/umac_if/src/fmac_vif.c \
	   fw_if/umac_if/src/fmac_util.c \
	   fw_if/umac_if/src/rx.c \
	   fw_if/umac_if/src/tx.c \
	   nrf_wifi_osal_module.c

ifeq ($(BUS_IF), QSPI)
	SRCS += bus_if/bus/qspi/src/qspi.c
else ifeq ($(BUS_IF), SPI)
	SRCS += bus_if/bus/pcie/src/spi.c
else ifeq ($(BUS_IF), PCIE)
	SRCS += bus_if/bus/pcie/src/pcie.c
endif

ifeq ($(RADIO_TEST), 1)
	SRCS += fw_if/umac_if/src/radio_test/fmac_api.c
	ccflags-y += -DNRF70_RADIO_TEST
	INCLUDES += -I$(NRF_WIFI_DIR)/fw_if/umac_if/inc/radio_test
else
	SRCS += fw_if/umac_if/src/default/fmac_api.c
	ccflags-y += -DNRF70_SYSTEM_MODE
	INCLUDES += -I$(NRF_WIFI_DIR)/fw_if/umac_if/inc/default
endif

ifeq ($(FW_LOAD_SUPPORT), 1)
ifeq ($(FW_LOAD), $(filter $(FW_LOAD), ROM ROM_PATCH))
SRCS += hw_if/hal/src/hal_fw_rom_loader.c
endif
ifeq ($(FW_LOAD), $(filter $(FW_LOAD), PATCH ROM_PATCH))
SRCS += hw_if/hal/src/hal_fw_patch_loader.c
endif
ifeq ($(FW_LOAD), $(filter $(FW_LOAD), RAM))
SRCS += hw_if/hal/src/hal_fw_ram_loader.c
endif
endif

ccflags-y += $(INCLUDES)
# Object files
OBJS = $(SRCS:.c=.o)

# Module
obj-m := nrf_wifi_osal.o
nrf_wifi_osal-objs := $(OBJS)

all:
	@echo "Building nrf_wifi_osal module: $(NRF_WIFI_DIR)"
	$(MAKE) -C $(KDIR) M=$(NRF_WIFI_DIR) modules

menuconfig:
	$(MAKE) -C $(KDIR) M=$(NRF_WIFI_DIR) menuconfig

clean:
	make -C $(KDIR) M=$(NRF_WIFI_DIR) clean

install:
	$(MAKE) -C $(KDIR) M=$(NRF_WIFI_DIR) modules_install

.PHONY: all clean install menuconfig
