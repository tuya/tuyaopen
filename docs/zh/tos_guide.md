# tos 使用指南
tos 命令是 tuyaopen 构建工具，支持创建、编译、配置等多种功能。

tos 命令位于 [tuyaopen](https://github.com/tuya/tuyaopen.git) 根目录下，使用 shell 脚本实现，使用前先将 tuyaopen 路径添加到系统环境变量中。

## 命令列表
可通过 `tos help` 查看命令列表。
```shell
$ tos help

Usage: tos COMMAND [ARGS]...

Commands:
    version    - Show TOS verson
    check      - Check command and version
    new        - New project
    build      - Build specific project or all projects
                 [name] (default build all project by project_build.ini)
    clean      - Clean specific project or all projects
                 [name] (default clean all project by project_build.ini)
    menuconfig - Configuration project features
    build_all  - Build all projects in the directory
                 [*path]
    help       - Help information
```

## 使用示例

### 查看版本
```shell
$ tos version
1.0.0
```

### 检测当前环境
```shell
$ tos check
Check command and version ...
Check [python3](3.10.12) > [3.6.0]: OK.
Check [git](2.34.1) > [2.0.0]: OK.
Check [cmake](3.22.1) > [3.16.0]: OK.
Check [ccmake](3.22.1) > [3.16.0]: OK.
Check [lcov](1.14) > [1.14]: OK.
```

tos check 命令会检测当前环境是否满足构建要求，如果满足则输出 OK，不符合会提示所需最低版本如 `Please install [lcov], and version > [1.14]`，请根据检测结果，安装相关依赖及对应版本。

### 创建项目
```shell
$ tos new
```
1. 根据提示输入项目名称，如： `hello_world`。
2. 选择项目对应的 platform：
```shell
Choice platform ...
========================
Platforms
  1. t2
  2. t3
  3. ubuntu
------------------------
Please select: 
```
选择完成后，tos 会自动下载相关依赖文件，并生成项目目录及模板文件。
```shell
├── CMakeLists.txt
├── project_build.ini
└── src
    └── hello_world.c
```
其中：
- `CMakeLists.txt`：项目配置文件，用于配置项目编译选项。
- `project_build.ini`：项目配置文件，用于配置项目编译选项。
其中：
    - project 名为工程名_platform 名
    - platform 为工程对应的平台。
- `src`：源码目录，用于存放项目源码文件。
- `src/hello_world.c`：项目源码文件，用于存放项目源码。

### 编译项目

进入项目目录，执行以下命令：
```shell
$ cd hello_world
$ tos build
```

### 配置项目
进入项目目录，执行以下命令：
```shell
$ cd hello_world
$ tos menuconfig
```

### 清理项目
进入项目目录，执行以下命令：
```shell
$ cd hello_world
$ tos clean
```

## 编译指定目录下所有项目
```shell
$ tos build_all .
```
tos 将根据项目工程下的 project_build.ini 文件，依次编译指定目录下所有工程配置文件下的所有项目。