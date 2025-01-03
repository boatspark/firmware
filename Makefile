#################################################################
# Makefile to build Particle IOT firmware.
#
# Designed to run on WSL. Will download gcc-arm and device-os.
# Works with x64 and aarch64.
# 
# Must install the following packages first:
# sudo apt-get install -y git bash curl bats bzip2 isomd5sum jq \
# libarchive-zip-perl  make zip wget parallel gnupg cmake xxd
#
# Flash currently depends on installing particle.exe CLI.
#
#################################################################

#################################
# Configuration Settings
export DEVICE_OS_VERSION=v6.1.0
export PLATFORM=boron
DEVICE_NAME=boron
#################################

# Determine location and name of current folder
export APPDIR := $(abspath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
export TARGET_FILE:=$(notdir $(APPDIR))

# Common directory to install gcc-arm and device-os so that they
# can be shared by related projects
SHAREDIR=$(abspath $(HOME))/.local/share/boatspark

# Install and build settings
DEVICE_OS_PATH=$(SHAREDIR)/device-os/$(DEVICE_OS_VERSION)
LOCALARCH=$(shell uname -m)
GCC_ARM_PATH=$(SHAREDIR)/gcc-arm
export PATH:=$(GCC_ARM_PATH)/bin:$(PATH)
MAKEFLAGS += --no-print-directory

# Source files and target used to determine if a new build is required
sources:=$(wildcard src/*.[ch]??)
target:=./target/$(TARGET_FILE).bin

build: gcc-arm device-os compile

compile: $(target)

# Build target if sources have changed
$(target): $(sources) 
	# Run Device OS makefile to make the user part
	cd "$(DEVICE_OS_PATH)/main" && make all

clean:
	/bin/rm -rf ./target

rebuild: clean build

# Flash currently uses Windows particle command not Linux binary
flash:
	particle.exe flash --local $(DEVICE_NAME) $(target)

buildflash: build flash

monitor:
	particle.exe serial monitor --follow

# Check gcc-arm is available and download if not present
gcc-arm:
ifeq (,$(wildcard $(GCC_ARM_PATH)/bin/arm-none-eabi-gcc))
	mkdir -p "$(GCC_ARM_PATH)" && curl -sSL "https://developer.arm.com/-/media/Files/downloads/gnu-rm/10-2020q4/gcc-arm-none-eabi-10-2020-q4-major-$(LOCALARCH)-linux.tar.bz2" | tar xjvf - --strip-components=1 -C "$(GCC_ARM_PATH)"
endif

# Download device-os if not present
device-os:
ifeq (,$(wildcard $(DEVICE_OS_PATH)/main/makefile))
	git clone https://github.com/particle-iot/device-os.git $(DEVICE_OS_PATH) && cd $(DEVICE_OS_PATH) && git checkout tags/$(DEVICE_OS_VERSION) && git submodule update --init
endif

help:
	@echo '┌──────────++++'
	@echo '│ Current Configuration'
	@echo '│   $$APPDIR           $(APPDIR)'
	@echo '│   $$DEVICE_OS_PATH   $(DEVICE_OS_PATH)'
	@echo '│   $$GCC_ARM_PATH     $(GCC_ARM_PATH)'
	@echo '│   $$LOCALARCH        $(LOCALARCH)'
	@echo '│   $$TARGET_FILE      $(TARGET_FILE)'
	@echo '│   $$PLATFORM         $(PLATFORM)'
