
SDK_ROOT := ~/nvidia/r36.4.4
SDK_GCC := ~/nvidia/r36.4.4/14t-bin
SYSROOT := ~/nvidia/softwork/sysroot

# TARGET_IP ?= 192.168.1.119
TARGET_IP ?= 192.168.1.193
USR := bp#自定上传到 /home/$(USR)/nfs 文件夹里面
PASSWORD := Cr123789
REMOTE_PATH ?= /home/bp/nfs

export TARGET_IP USR PASSWORD

# ssh-keygen -f "/home/yz/.ssh/known_hosts" -R "192.168.1.13"

CROSS_COMPILER := $(SDK_GCC)/bin/aarch64-buildroot-linux-gnu-
KERNEL_DIR := $(SDK_ROOT)/Linux_for_Tegra/source/kernel/kernel-jammy-src
EXTRA_CFLAGS += -I$(SDK_ROOT)/Linux_for_Tegra/source/nvidia-oot/include
EXTRA_CFLAGS += -I$(SDK_ROOT)/Linux_for_Tegra/source/out/nvidia-conftest
KBUILD_EXTRA_SYMBOLS := $(SDK_ROOT)/Linux_for_Tegra/source/nvidia-oot/Module.symvers

# ==================================================
# SCP upload function
# ==================================================

define SCP_UPLOAD
	echo "[INFO] Uploading $(1) -> $(TARGET_IP):$(REMOTE_PATH)"; \
	sshpass -p "$(PASSWORD)" scp \
		-o StrictHostKeyChecking=no \
		-o UserKnownHostsFile=/dev/null \
		-o LogLevel=ERROR \
		"$(1)" \
		$(USR)@$(TARGET_IP):$(REMOTE_PATH)
endef

# ==================================================
# Check network and upload
# ==================================================
define CHECK_AND_UPLOAD
	@if ping -c 1 -W 1 $(TARGET_IP) >/dev/null 2>&1; then \
		$(call SCP_UPLOAD,$(1)); \
	else \
		echo "[WARN] $(TARGET_IP) unreachable, skip upload: $(1)"; \
	fi
endef