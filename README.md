# C++ Cross-Compile Shared Library Template

## 结构说明

- `include/`：头文件目录，包含主库和子类库头文件
- `src/`：源文件目录，包含主库和子类库实现
- `test/`：测试程序目录，包含 main.cpp
- `Makefile`：支持交叉编译和 sysroot 的构建脚本

## 主要特性
- 支持交叉工具链和 sysroot 配置
- 动态库（.so）模板，主库和可扩展子类接口
- 测试程序 main.cpp
- 易于扩展新子类

## 使用方法
1. 修改 `Makefile` 中的 `CXX` 和 `SYSROOT` 变量为你的交叉工具链和 sysroot 路径。
2. 执行 `make` 编译动态库。
3. 执行 `make test` 编译测试程序。
4. 运行测试程序：`./test/test_app`

## 测试
* 输出成果物:`device.h`和`dev_common.h`文件和库文件`device.so`库文件
```bash
./test_app
```