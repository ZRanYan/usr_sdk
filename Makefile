
THIS_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
PROJECT_ROOT ?= $(abspath $(THIS_DIR)/..)
$(info PROJECT_ROOT = $(PROJECT_ROOT))
include $(PROJECT_ROOT)/config.mk


PROGRAM_VER := 0.0.5.5
LIB_NAME := device
REAL_LIB_RELEASE := lib$(LIB_NAME)_$(PROGRAM_VER).so
REAL_LIB_DEBUG := lib$(LIB_NAME)_$(PROGRAM_VER)_debug.so
SONAME   := lib$(LIB_NAME).so

# DEBUG flag, can be set via command line: make DEBUG=1
DEBUG ?= 0

LIB_SEARCH_PATH ?= ./
RUNTIME_RPATH   ?= $(LIB_SEARCH_PATH):\$$ORIGIN # ORIGIN 代表程序运行存储路径信息

CXX := $(CROSS_COMPILER)g++
CXXFLAGS_RELEASE := -Wall -fPIC --sysroot=$(SYSROOT) -Iinclude -I$(SYSROOT)/usr/include/aarch64-linux-gnu \
			-DPROGRAM_VER=\"$(PROGRAM_VER)\" 
CXXFLAGS_DEBUG := $(CXXFLAGS_RELEASE) -DDEBUG_LOG_OPEN

LDFLAGS += -O2 -Wall -Wextra -Wl,--enable-new-dtags
LDFLAGS += -Wl,-rpath,$(RUNTIME_RPATH) 
LDFLAGS += -Wl,--allow-shlib-undefined

LDFLAGS_SO := -shared -L$(SYSROOT)/lib -L$(SYSROOT)/usr/lib -L$(SYSROOT)/usr/lib/aarch64-linux-gnu
LDFLAGS_APP := \
	-Wl,-rpath-link,$(SYSROOT)/usr/lib \
	-Wl,-rpath-link,$(SYSROOT)/usr/lib/aarch64-linux-gnu \
	-Wl,-rpath-link,$(SYSROOT)/lib/aarch64-linux-gnu \
	-L$(SYSROOT)/usr/local/lib \
	-L$(SYSROOT)/usr/lib/aarch64-linux-gnu \
	-L$(SYSROOT)/usr/lib/aarch64-linux-gnu/tegra \
	-L$(SYSROOT)/lib/aarch64-linux-gnu \
	-L$(SYSROOT)/usr/lib \
	-L$(SYSROOT)/lib \
	-lcjson -lpthread \
	-lopencv_core \
	-lopencv_imgproc \
	-lopencv_imgcodecs 

SRC = $(wildcard src/*.cpp)
OBJ_RELEASE = $(SRC:.cpp=.release.o)
OBJ_DEBUG = $(SRC:.cpp=.debug.o)

.PHONY: all prepare clean release debug

all: release debug symlink test

release: $(REAL_LIB_RELEASE)

debug: $(REAL_LIB_DEBUG)

prepare:
	@./generate_version_header.sh

# Release object files
src/%.release.o: src/%.cpp prepare
	@$(CXX) $(CXXFLAGS_RELEASE) -c $< -o $@

# Debug object files  
src/%.debug.o: src/%.cpp prepare
	@$(CXX) $(CXXFLAGS_DEBUG) -c $< -o $@

# Release library
$(REAL_LIB_RELEASE): $(OBJ_RELEASE)
	@$(CXX) $(CXXFLAGS_RELEASE) -shared -o $@ $^ $(LDFLAGS_SO)

# Debug library
$(REAL_LIB_DEBUG): $(OBJ_DEBUG)
	@$(CXX) $(CXXFLAGS_DEBUG) -shared -o $@ $^ $(LDFLAGS_SO)

symlink:
	@ln -sf $(REAL_LIB_RELEASE) $(SONAME)
	@$(CXX) $(CXXFLAGS_RELEASE) $(LDFLAGS) test/main.cpp test/test_func.cpp -o test/test_app -L. -ldevice $(LDFLAGS_APP)

.PHONY: clean test upload

test: $(SONAME)
	@rm -f src/*.o  test/test_app
	@$(CXX) $(CXXFLAGS_RELEASE) $(LDFLAGS) test/main.cpp test/test_func.cpp -o test/test_app -L. -ldevice $(LDFLAGS_APP)
	@if ping -c 1 -W 1 $(TARGET_IP) >/dev/null 2>&1; then \
		echo "Network reachable, uploading test_app"; \
		sshpass -p 'Cr123789' scp ./test/test_app bp@$(TARGET_IP):/home/bp/nfs; \
		sshpass -p 'Cr123789' scp ./$(SONAME) bp@$(TARGET_IP):/home/bp/nfs; \
		sshpass -p 'Cr123789' scp ./$(REAL_LIB_RELEASE) bp@$(TARGET_IP):/home/bp/nfs; \
		sshpass -p 'Cr123789' scp ./$(REAL_LIB_DEBUG) bp@$(TARGET_IP):/home/bp/nfs; \
	else \
		echo "Network unreachable, skip upload"; \
	fi

upload:
	sshpass -p 'Cr123789' scp ./test/test_app bp@$(TARGET_IP):/home/bp/nfs
	sshpass -p 'Cr123789' scp ./$(SONAME) bp@$(TARGET_IP):/home/bp/nfs
	sshpass -p 'Cr123789' scp ./$(REAL_LIB_RELEASE) bp@$(TARGET_IP):/home/bp/nfs
	sshpass -p 'Cr123789' scp ./$(REAL_LIB_DEBUG) bp@$(TARGET_IP):/home/bp/nfs
clean:
	@rm -f src/*.o src/*.release.o src/*.debug.o *.so test/test_app
