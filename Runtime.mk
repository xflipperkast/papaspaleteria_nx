.SUFFIXES:
.DEFAULT_GOAL := all
ifeq ($(strip $(DEVKITPRO)),)
$(error DEVKITPRO is not set; export DEVKITPRO=/opt/devkitpro)
endif
TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

TARGET := paleteria_runtime
BUILD := .cache/build
SOURCES := source
INCLUDES := include
ROMFS := .cache/runtime-romfs
APP_TITLE := Papa's Paleteria To Go!
APP_AUTHOR := xFlippy
APP_VERSION := 1.0.0
export SOURCE_DATE_EPOCH ?= 1792022400
ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CFLAGS := -g -Os -Wall -Wextra -Wno-unused-function -ffunction-sections -fdata-sections $(ARCH) -DAPK64_HAVE_VORBIS
CFLAGS += $(INCLUDE) -D__SWITCH__ `$(PREFIX)pkg-config --cflags freetype2`
CFLAGS += -ffile-prefix-map=$(TOPDIR)=. -fmacro-prefix-map=$(TOPDIR)=.
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS := -g $(ARCH)
LDFLAGS := -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(TARGET).map
LDFLAGS += -Wl,--wrap=_malloc_r -Wl,--wrap=_free_r -Wl,--wrap=_realloc_r -Wl,--wrap=_calloc_r -Wl,--wrap=_memalign_r -Wl,--wrap=_malloc_usable_size_r -Wl,--wrap=__libnx_exception_entry
LIBS := -Wl,--start-group -lEGL -lGLESv2 -lglapi -ldrm_nouveau -lvorbisfile -lvorbis -logg `$(PREFIX)pkg-config --libs freetype2` -lcurl -lmbedtls -lmbedx509 -lmbedcrypto -lz -Wl,--end-group -lstdc++ -lnx -lm
LIBDIRS := $(TOPDIR)/.cache/mesa-switch $(TOPDIR)/.cache/curl-switch $(PORTLIBS) $(LIBNX)

ifneq ($(notdir $(BUILD)),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(BUILD)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)
CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
export LD := $(CC)
export OFILES := $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) $(SFILES:.s=.o)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export NROFLAGS := --icon=$(CURDIR)/icon.jpg --nacp=$(CURDIR)/$(BUILD)/$(TARGET).nacp --romfsdir=$(CURDIR)/$(ROMFS)
export ROMFS_FILES := $(shell find $(CURDIR)/$(ROMFS) -type f)

.PHONY: all clean dependencies check package
all: dependencies

dependencies:
	@bash tools/mesa/build_switch.sh
	@bash tools/libcurl/build_switch.sh
	@mkdir -p $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Runtime.mk

check:
	@bash tests/run.sh

package: all
	@python3 tools/package_release.py

clean:
	rm -rf $(BUILD) $(TARGET).nro
else
LDFLAGS += -Wl,--build-id=0x$(shell python3 $(TOPDIR)/tools/build_id.py)
DEPENDS := $(OFILES:.o=.d)
.PHONY: all
all: $(OUTPUT).nro
$(OUTPUT).nro: $(OUTPUT).elf $(OUTPUT).nacp $(ROMFS_FILES) $(TOPDIR)/Runtime.mk $(TOPDIR)/icon.jpg
$(OUTPUT).nacp: $(TOPDIR)/Runtime.mk
$(OUTPUT).elf: $(OFILES) $(TOPDIR)/.cache/mesa-switch/lib/libEGL.a $(TOPDIR)/.cache/curl-switch/lib/libcurl.a $(TOPDIR)/tools/build_id.py
$(OFILES): $(TOPDIR)/Runtime.mk
runtime_database.o: CFLAGS += -w
-include $(DEPENDS)
endif
