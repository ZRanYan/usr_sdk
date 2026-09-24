#!/bin/bash
set -e

# 脚本所在目录（device_sdk）
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="${SCRIPT_DIR}/libdevice"

# 清理旧输出目录（如果需要）
rm -rf "${OUTPUT_DIR}"
mkdir -p "${OUTPUT_DIR}"

# 1. 编译当前 Makefile
echo ">>> Start building..."
make -C "${SCRIPT_DIR}" clean 2>/dev/null || true
make -C "${SCRIPT_DIR}"
echo ">>> Build completed."

# 2. 查找生成的 libdevic*.so 库文件（支持多个匹配）
shopt -s nullglob
LIB_FILES=("${SCRIPT_DIR}"/libdevic*.so)
if [ ${#LIB_FILES[@]} -eq 0 ]; then
    echo "ERROR: No libdevic*.so found in ${SCRIPT_DIR}" >&2
    exit 1
fi

echo ">>> Found libraries:"
for lib in "${LIB_FILES[@]}"; do
    echo "   - $(basename "$lib")"
done

# 3. 拷贝库文件到输出目录
cp "${LIB_FILES[@]}" "${OUTPUT_DIR}/"

# 4. 拷贝 include 目录下的指定头文件
INCLUDE_DIR="${SCRIPT_DIR}/include"
if [ -d "${INCLUDE_DIR}" ]; then
    for hdr in dev_common.h device.h; do
        if [ -f "${INCLUDE_DIR}/${hdr}" ]; then
            cp "${INCLUDE_DIR}/${hdr}" "${OUTPUT_DIR}/"
        else
            echo "WARNING: ${hdr} not found in ${INCLUDE_DIR}" >&2
        fi
    done
else
    echo "WARNING: include directory not found at ${INCLUDE_DIR}" >&2
fi

cp -f ./version.mk "${OUTPUT_DIR}/version.h"

#拷贝test_app文件
cp -f "${SCRIPT_DIR}/test/test_app"   "${OUTPUT_DIR}/test_app"

# 5. 生成 checklist.txt
CHECKLIST="${OUTPUT_DIR}/checklist.txt"

# 获取 Git 信息（兼容无 git 目录或非 git 仓库）
GIT_REMOTE=""
GIT_BRANCH=""
GIT_COMMIT=""
if command -v git &>/dev/null && git -C "${SCRIPT_DIR}" rev-parse --git-dir &>/dev/null; then
    GIT_REMOTE=$(git -C "${SCRIPT_DIR}" remote get-url origin 2>/dev/null || echo "unknown")
    GIT_BRANCH=$(git -C "${SCRIPT_DIR}" rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")
    GIT_COMMIT=$(git -C "${SCRIPT_DIR}" rev-parse HEAD 2>/dev/null || echo "unknown")
else
    GIT_REMOTE="no git repository"
    GIT_BRANCH="no git repository"
    GIT_COMMIT="no git repository"
fi

# 当前日期
BUILD_DATE=$(date "+%Y-%m-%d %H:%M:%S")

# 计算输出目录下所有文件的 MD5（排序保证顺序一致）
echo "Build Information:" > "${CHECKLIST}"
echo "  Date       : ${BUILD_DATE}" >> "${CHECKLIST}"
echo "  Git Remote : ${GIT_REMOTE}" >> "${CHECKLIST}"
echo "  Git Branch : ${GIT_BRANCH}" >> "${CHECKLIST}"
echo "  Git Commit : ${GIT_COMMIT}" >> "${CHECKLIST}"
echo "" >> "${CHECKLIST}"
echo "Files MD5 Checksums:" >> "${CHECKLIST}"

# 计算每个文件的 MD5（排除 checklist.txt 自身）
for f in "${OUTPUT_DIR}"/*; do
    if [ -f "$f" ]; then
        filename=$(basename "$f")
        # 跳过 checklist.txt 自身
        if [ "$filename" = "checklist.txt" ]; then
            continue
        fi
        md5=$(md5sum "$f" | awk '{print $1}')
        printf "  %-30s %s\n" "${filename}" "${md5}" >> "${CHECKLIST}"
    fi
done

echo ""
echo ">>> Build package successfully created at: ${OUTPUT_DIR}"
echo ">>> Checklist: ${CHECKLIST}"

# 获取当前时间戳，格式：YYYYMMDDHHMMSS（注意：秒是两位数字，总共14位）
TIMESTAMP=$(date "+%Y%m%d%H%M%S")
ZIP_NAME="libdevice_${TIMESTAMP}.zip"
ZIP_PATH="${SCRIPT_DIR}/${ZIP_NAME}"

echo ""
echo ">>> Creating zip archive..."

# 进入 SCRIPT_DIR 以便 zip 的相对路径正确（只打包 libdevice 目录）
cd "${SCRIPT_DIR}"

# 检查是否安装了 zip
if command -v zip &>/dev/null; then
    # -r 递归，-q 安静模式（可选去掉 -q 以便看到进度）
    zip -r "${ZIP_PATH}" "libdevice/" -x "libdevice/.*" 2>/dev/null
    echo ">>> Zip archive created: ${ZIP_PATH}"
else
    echo "WARNING: 'zip' command not found. Trying 'tar' as fallback..."
    # 如果没有 zip，使用 tar + gzip 作为备用
    TAR_NAME="libdevice_${TIMESTAMP}.tar.gz"
    tar -czf "${TAR_NAME}" "libdevice/"
    echo ">>> Tar archive created: ${SCRIPT_DIR}/${TAR_NAME}"
fi

echo ""
echo "=== All done ==="

