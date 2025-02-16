# tos Guide

The tos command is a build tool for tuyaopen, supporting various functions such as creating, compiling, and configuring.

The tos command is located in the root directory of [tuyaopen](https://github.com/tuya/tuyaopen.git) and is implemented using shell scripts. Before using it, you need to add the path of tuyaopen to the system environment variable.

## Command List
You can view the command list by running `tos help`.

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

## Usage Examples

### View Version
```shell
$ tos version
1.0.0
```

### Check Current Environment
```shell
$ tos check
Check command and version ...
Check [python3](3.10.12) > [3.6.0]: OK.
Check [git](2.34.1) > [2.0.0]: OK.
Check [cmake](3.22.1) > [3.16.0]: OK.
Check [ccmake](3.22.1) > [3.16.0]: OK.
Check [lcov](1.14) > [1.14]: OK.
```

The tos check command will check whether the current environment meets the build requirements. If it does, it will output OK. If not, it will prompt for the minimum version required, such as `Please install [lcov], and version > [1.14]`. Please install the relevant dependencies and corresponding versions according to the check results.

### Create Project
```shell
$ tos new
```
1. Enter the project name according to the prompt, such as: `hello_world`.
2. Select the platform corresponding to the project:
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
After the selection, tos will automatically download the relevant dependency files and generate the project directory and template files.
```shell
├── CMakeLists.txt
├── project_build.ini
└── src
    └── hello_world.c
```
Among them:
- `CMakeLists.txt`: Project configuration file, used to configure project compilation options.
- `project_build.ini`: Project configuration file, used to configure project compilation options.
    - The project name is the project name + platform name.
    - The platform is the platform corresponding to the project.
- `src`: Source code directory, used to store project source code files.
- `src/hello_world.c`: Project source code file, used to store project source code.

### Compile Project

Enter the project directory and run the following command:
```shell
$ cd hello_world
$ tos build
```

### Configure Project
Enter the project directory and run the following command:
```shell
$ cd hello_world
$ tos menuconfig
```

### Clean Project
Enter the project directory and run the following command:
```shell
$ cd hello_world
$ tos clean
```

## Compile All Projects in the Specified Directory
```shell
$ tos build_all .
```
Tos will compile all projects in the specified directory according to the project configuration files under the project engineering directory.
