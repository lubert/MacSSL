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
.PHONY: all build clean run lib help

all: build

# Build the demo application
build:
	@echo "Building MacSSL with Retro68 toolchain..."
	@if [ ! -f "$(TOOLCHAIN_PATH)" ]; then \
		echo "Error: Retro68 toolchain file not found at: $(TOOLCHAIN_PATH)"; \
		echo "Please verify RETRO68_BUILD_ROOT points to a valid Retro68 build directory"; \
		exit 1; \
	fi
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && $(CMAKE) .. -DCMAKE_TOOLCHAIN_FILE="$(TOOLCHAIN_PATH)"
	@cd $(BUILD_DIR) && $(MAKE)
	@echo "Build successful!"
	@echo "Output files:"
	@ls -la $(BUILD_DIR)/*.dsk $(BUILD_DIR)/*.bin $(BUILD_DIR)/MacSSL* 2>/dev/null || echo "No output files found"

# Clean build artifacts
clean:
	@echo "Cleaning build directory..."
	@rm -rf $(BUILD_DIR)
	@echo "Clean complete."

# Run the application using LaunchAPPL
run: build
	@echo "Running MacSSL application..."
	@if [ -f "$(BUILD_DIR)/MacSSL.code.bin" ]; then \
		cd $(BUILD_DIR) && LaunchAPPL MacSSL.code.bin; \
	else \
		echo "Error: MacSSL.code.bin not found. Please run 'make build' first."; \
		exit 1; \
	fi

# Build just the library (future: create static library)
lib:
	@echo "Building MacSSL library components..."
	@echo "Note: Full library target not yet implemented"
	@echo "Currently builds complete demo application"
	@$(MAKE) build

# Show help
help:
	@echo "MacSSL Makefile - Classic Mac OS TLS Library"
	@echo ""
	@echo "Targets:"
	@echo "  build  - Build the MacSSL demo application"
	@echo "  clean  - Clean build artifacts"
	@echo "  run    - Build and run the application via LaunchAPPL"
	@echo "  lib    - Build library components"
	@echo "  help   - Show this help message"
	@echo ""
	@echo "Requirements:"
	@echo "  - RETRO68_BUILD_ROOT environment variable must be set"
	@echo "  - Retro68 toolchain must be installed"
	@echo "  - LaunchAPPL must be available for 'run' target"