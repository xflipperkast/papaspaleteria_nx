.DEFAULT_GOAL := all
ifeq ($(strip $(DEVKITPRO)),)
$(error DEVKITPRO is not set; export DEVKITPRO=/opt/devkitpro)
endif
DEVKITA64 ?= $(DEVKITPRO)/devkitA64
CC := $(DEVKITA64)/bin/aarch64-none-elf-gcc
ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
export SOURCE_DATE_EPOCH ?= 1792022400
.PHONY: all runtime check package clean
all: runtime
runtime:
	python3 tools/unpack_support.py
	python3 tools/prepare_romfs.py
	$(MAKE) --no-print-directory -f Runtime.mk
	python3 tools/pack_runtime.py
	@mkdir -p .cache/bootstrap
	$(CC) -g -Os -Wall -Wextra -Werror -ffunction-sections -fdata-sections $(ARCH) \
	 -D__SWITCH__ -I$(DEVKITPRO)/libnx/include -I$(DEVKITPRO)/portlibs/switch/include \
	 -I.cache/generated -Ibootstrap -ffile-prefix-map=$(CURDIR)=. -fmacro-prefix-map=$(CURDIR)=. \
	 -specs=$(DEVKITPRO)/libnx/switch.specs -Wl,--gc-sections \
	 bootstrap/loader.c -L$(DEVKITPRO)/portlibs/switch/lib -L$(DEVKITPRO)/libnx/lib \
	 -lz -lnx -o .cache/bootstrap/loader.elf
	$(DEVKITPRO)/tools/bin/nacptool --create "Papa's Paleteria To Go!" xFlippy 1.2.4 .cache/bootstrap/loader.nacp
	$(DEVKITPRO)/tools/bin/elf2nro .cache/bootstrap/loader.elf papaspaleteria_nx.nro \
	 --icon=icon.jpg --nacp=.cache/bootstrap/loader.nacp --romfsdir=.cache/packed-romfs
	python3 tools/audit.py
check:
	python3 tools/unpack_support.py
	bash tests/run.sh
package: all
	python3 tools/package_release.py
clean:
	rm -rf .cache/build .cache/bootstrap .cache/generated .cache/packed-romfs .cache/runtime-romfs papaspaleteria_nx.nro
