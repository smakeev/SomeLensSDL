BUILD_DIR ?= build
CONFIGURATION ?= Release
JOBS ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

.PHONY: all build run shaders install_tools install_tools_debian install_tools_macos clean help

all: build

help:
	@printf '%s\n' \
		'  make install_tools   install what building needs (apt on Debian/Ubuntu, Homebrew on macOS)' \
		'  make build           build the library and the demo' \
		'  make run             build and start the demo' \
		'  make shaders         recompile shaders/*.frag into src/generated (needs glslc and spirv-cross)' \
		'  make clean           remove the build'

build:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(CONFIGURATION)
	cmake --build $(BUILD_DIR) -j$(JOBS)

run: build
	./$(BUILD_DIR)/somelens_demo

shaders:
	./shaders/build.sh

clean:
	cmake -E rm -rf $(BUILD_DIR)

install_tools:
	@if command -v brew >/dev/null 2>&1; then \
		$(MAKE) --no-print-directory install_tools_macos; \
	elif command -v apt-get >/dev/null 2>&1; then \
		$(MAKE) --no-print-directory install_tools_debian; \
	else \
		echo "unsupported platform: install cmake, SDL3 (3.4+), SDL3_ttf, glslc and spirv-cross by hand"; \
		exit 1; \
	fi

install_tools_debian:
	@missing=""; \
	for package in cmake build-essential libsdl3-dev libsdl3-ttf-dev glslc spirv-cross xxd; do \
		dpkg -s "$$package" >/dev/null 2>&1 || missing="$$missing $$package"; \
	done; \
	if [ -z "$$missing" ]; then \
		echo "all packages already installed"; \
	else \
		echo "installing:$$missing"; \
		sudo apt-get update && sudo apt-get install -y $$missing; \
	fi

install_tools_macos:
	@for formula in cmake sdl3 sdl3_ttf shaderc spirv-cross; do \
		if brew list --versions "$$formula" >/dev/null 2>&1; then \
			echo "$$formula: already installed"; \
		else \
			echo "$$formula: installing"; \
			brew install "$$formula" || exit 1; \
		fi; \
	done
