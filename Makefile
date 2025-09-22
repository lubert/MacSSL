# Makefile for MacSSL - Classic Mac OS TLS Library
# Uses Retro68 GCC toolchain for cross-compilation

# Check for required environment variable
ifndef RETRO68_BUILD_ROOT
$(error RETRO68_BUILD_ROOT environment variable is not set. Please set it to your Retro68 build directory)
endif

# Build configuration
BUILD_DIR = build
TOOLCHAIN_PATH = $(RETRO68_BUILD_ROOT)/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
CMAKE = cmake
MAKE = make

# Default target
.PHONY: all build demo lib clean run help

all: demo

# Build the library only
lib:
	@echo "Building MacSSL library with Retro68 toolchain..."
	@if [ ! -f "$(TOOLCHAIN_PATH)" ]; then \
		echo "Error: Retro68 toolchain file not found at: $(TOOLCHAIN_PATH)"; \
		echo "Please verify RETRO68_BUILD_ROOT points to a valid Retro68 build directory"; \
		exit 1; \
	fi
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && $(CMAKE) .. -DCMAKE_TOOLCHAIN_FILE="$(TOOLCHAIN_PATH)"
	@cd $(BUILD_DIR) && $(MAKE)
	@echo "Library build successful!"
	@echo "Library file:"
	@ls -la $(BUILD_DIR)/libMacSSL.a 2>/dev/null || echo "Library file not found"

# Build the demo application (default behavior)
demo:
	@echo "Building MacSSL demo application with Retro68 toolchain..."
	@if [ ! -f "$(TOOLCHAIN_PATH)" ]; then \
		echo "Error: Retro68 toolchain file not found at: $(TOOLCHAIN_PATH)"; \
		echo "Please verify RETRO68_BUILD_ROOT points to a valid Retro68 build directory"; \
		exit 1; \
	fi
	@mkdir -p demo/build
	@cd demo/build && $(CMAKE) .. -DCMAKE_TOOLCHAIN_FILE="$(TOOLCHAIN_PATH)"
	@cd demo/build && $(MAKE)
	@echo "Demo build successful!"
	@echo "Output files:"
	@ls -la demo/build/*.dsk demo/build/*.bin demo/build/PostMac* 2>/dev/null || echo "No output files found"

# Legacy alias for demo build
build: demo

# Clean build artifacts
clean:
	@echo "Cleaning build directories..."
	@rm -rf $(BUILD_DIR)
	@rm -rf demo/build
	@echo "Clean complete."

# Run the demo application using LaunchAPPL
run: demo
	@echo "Running PostMac demo application..."
	@if [ -f "demo/build/PostMac.code.bin" ]; then \
		cd demo/build && LaunchAPPL PostMac.code.bin; \
	else \
		echo "Error: PostMac.code.bin not found. Please run 'make demo' first."; \
		exit 1; \
	fi

# Show help
help:
	@echo "MacSSL Makefile - Classic Mac OS TLS Library"
	@echo ""
	@echo "Targets:"
	@echo "  demo   - Build the MacSSL demo application (default)"
	@echo "  lib    - Build the MacSSL library only"
	@echo "  build  - Legacy alias for demo build"
	@echo "  clean  - Clean all build artifacts"
	@echo "  run    - Build and run the demo application via LaunchAPPL"
	@echo "  help   - Show this help message"
	@echo ""
	@echo "Structure:"
	@echo "  Library build: Creates libMacSSL.a in build/"
	@echo "  Demo build:    Creates PostMac.APPL in demo/build/"
	@echo ""
	@echo "Requirements:"
	@echo "  - RETRO68_BUILD_ROOT environment variable must be set"
	@echo "  - Retro68 toolchain must be installed"
	@echo "  - LaunchAPPL must be available for 'run' target"