
#!/bin/bash

# 获取 Git commit hash
GIT_COMMIT=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")

# 获取当前分支名称
GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")

PROGRAM_DATE=$(date +"%Y-%m-%d %H:%M:%S" || echo "unknown")

# 生成版本头文件
echo "#ifndef VERSION_H" > ./include/version.h
echo "#define VERSION_H" >> ./include/version.h
echo "#define GIT_COMMIT \"${GIT_COMMIT}\"" >> ./include/version.h
echo "#define GIT_BRANCH \"${GIT_BRANCH}\"" >> ./include/version.h
echo "#define PROGRAM_DATE \"${PROGRAM_DATE}\"" >> ./include/version.h
echo "#endif // VERSION_H" >> ./include/version.h


