# tuyaopen
[English](https://github.com/tuya/tuyaopen/blob/master/README.md) | 简体中文

## 简介
tuyaopen 是一款跨芯片平台、操作系统的 IoT 开发框架。它基于通用南向接口设计，支持 Bluetooth、Wi-Fi、Ethernet 等通信协议，提供了物联网开发的核心功能，包括配网，激活，控制，升级等；它具备强大的安全合规能力，包括设备认证、数据加密、通信加密等，满足全球各个国家和地区的数据合规需求。

基于 tuyaopen 开发的 IoT 产品，如果使用 tuya_cloud_service 组件的功能，就可以使用涂鸦APP、云服务提供的强大生态能力，并与 Power By Tuya 设备互联互通。

同时 tuyaopen 将不断拓展，提供更多云平台接入功能，及语音、视频、人脸识别等功能。

## 开始体验

### 安装依赖
Ubuntu and Debian

```sh
$ sudo apt-get install lcov cmake-curses-gui build-essential wget git python3 python3-pip python3-venv libc6-i386 libsystemd-dev
```

### 克隆仓库

```sh
$ git clone https://github.com/tuya/tuyaopen.git
```

tuyeopen 仓库中包含多个子模块，tos 工具会在编译前检查并自动下载子模块，也可以使用 `git submodule update --init` 命令手工下载。

## 设置与编译

### step1. 设置环境变量
```sh
$ cd tuyaopen
$ export PATH=$PATH:$PWD
```
或将 tuyaopen 路径添加到系统环境变量中。

tuyaopen 通过 tos 命令进行编译、调试等操作，tos 命令会根据环境变量中设置的路径查找 tuyaopen 仓库，并执行对应操作。

tos 命令的详细使用方法，请参考 [tos 命令](./docs/zh/tos_guide.md)。

### step2. 设置 platform
tos 工具通过项目工程目录下的 `project_build.ini` 文件配置编译 platform，`project_build.ini` 包括以下字段：
- project: 项目名称，可自定义，建议工程目录名_<platform/chip name>。
- platform: 编译目标平台，可选值：ubuntu、t2、t3、t5、esp32、ln882h、bk7231x。该名称与 `platform/platform_config.yaml` 中定义的 name 名称一致。
- chip: 可选值，当 platform 中支持多 chip 时，需指定 chip 名称。
    - platform 为 esp32 时可选值：esp32、esp32c3。
    - platform 为 bk7231x 时可选值：bk7231n。

示例如下：
```bash
[project:sample_project_bk7231x]
platform = bk7231x
chip = bk7231n
```

同时 tos 工具可通过 `project_build.ini` 文件配置项目多平台同时编译，可参考[多平台配置](#多平台配置)。

### step3. 编译
选择当前编译的 examples 或 apps 对应工程，运行如下命令编译：
```shell
$ cd examples/get-started/sample_project
$ tos build
```
编译完成后目标文件位于当前编译项目 `.build/<project>/bin` 目录下，如 `examples/get-started/sample_project/.build/sample_project_t2/bin` 目录。
编译后的目标文件包括：
- sample_project_t2_QIO_1.0.0.bin：包括 boot 在内的完整固件，用于烧录。
- sample_project_t2_UA_1.0.0.bin：未包括 boot 的应用固件，使用该文件需根据不同的 platform/chip 烧录该 bin 至对应的地址，否则可能无法正常运行。
- sample_project_t2_UG_1.0.0.bin：用于 OTA 升级的 bin 文件，无法直接烧录后运行。


项目版本默认为 `1.0.0`，可在 menuconfig 配置中修改。

### step4. menuconfig 配置 
选择需配置的 examples 或 apps 对应工程，在对应工程目录下运行如下命令进行菜单化配置：
```sh
$ cd examples/get-started/sample_project
$ tos menuconfig
```
配置当前工程，配置完成后保存退出，编译工程。


## 多平台配置
tos 工具通过项目工程目录下的 `project_build.ini` 文件配置多平台编译，[多平台配置文件](examples/get-started/sample_project/project_build.ini) 格式如下：
```ini
[project:sample_project_t2]
platform = t2

[project:sample_project_t3]
platform = t3

[project:sample_project_ubuntu]
platform = ubuntu

[project:sample_project_t5]
platform = t5

[project:sample_project_esp32]
platform = esp32
chip = esp32c3                  # esp32/esp32c3 可选
```

默认 project 只有 1 个，如需编译多个 project，需在 `project_build.ini` 文件中添加多个 project 配置。

当配置文件中存在多个 project 时，`tos build` 命令会依次编译多个 project。

### 支持 platform 列表
| 名称 | 支持状态 | 介绍 | 调试日志串口 |
| ---- | ---- | ---- | ---- |
| Ubuntu | 支持 | 可在 ubuntu 等 Linux 主机上直接运行 | |
| T2 |  支持 | 支持模组列表:  [T2-U](https://developer.tuya.com/cn/docs/iot/T2-U-module-datasheet?id=Kce1tncb80ldq) | Uart2/115200 |
| T3 |  支持 | 支持模组列表:  [T3-U](https://developer.tuya.com/cn/docs/iot/T3-U-Module-Datasheet?id=Kdd4pzscwf0il) [T3-U-IPEX](https://developer.tuya.com/cn/docs/iot/T3-U-IPEX-Module-Datasheet?id=Kdn8r7wgc24pt) [T3-2S](https://developer.tuya.com/cn/docs/iot/T3-2S-Module-Datasheet?id=Ke4h1uh9ect1s) [T3-3S](https://developer.tuya.com/cn/docs/iot/T3-3S-Module-Datasheet?id=Kdhkyow9fuplc) [T3-E2](https://developer.tuya.com/cn/docs/iot/T3-E2-Module-Datasheet?id=Kdirs4kx3uotg) 等 | Uart1/460800 |
| T5 |  支持 | 支持模组列表: [T5-E1](https://developer.tuya.com/cn/docs/iot/T5-E1-Module-Datasheet?id=Kdar6hf0kzmfi) [T5-E1-IPEX](https://developer.tuya.com/cn/docs/iot/T5-E1-IPEX-Module-Datasheet?id=Kdskxvxe835tq) 等 | Uart1/460800 |
| ESP32/ESP32C3 | 支持 | | Uart0/115200 |
| LN882H | 支持 |  | Uart1/921600 |
| BK7231N | 支持 | 支持模组列表:  [CBU](https://developer.tuya.com/cn/docs/iot/cbu-module-datasheet?id=Ka07pykl5dk4u)  [CB3S](https://developer.tuya.com/cn/docs/iot/cb3s?id=Kai94mec0s076) [CB3L](https://developer.tuya.com/cn/docs/iot/cb3l-module-datasheet?id=Kai51ngmrh3qm) [CB3SE](https://developer.tuya.com/cn/docs/iot/CB3SE-Module-Datasheet?id=Kanoiluul7nl2) [CB2S](https://developer.tuya.com/cn/docs/iot/cb2s-module-datasheet?id=Kafgfsa2aaypq) [CB2L](https://developer.tuya.com/cn/docs/iot/cb2l-module-datasheet?id=Kai2eku1m3pyl) [CB1S](https://developer.tuya.com/cn/docs/iot/cb1s-module-datasheet?id=Kaij1abmwyjq2) [CBLC5](https://developer.tuya.com/cn/docs/iot/cblc5-module-datasheet?id=Ka07iqyusq1wm) [CBLC9](https://developer.tuya.com/cn/docs/iot/cblc9-module-datasheet?id=Ka42cqnj9r0i5) [CB8P](https://developer.tuya.com/cn/docs/iot/cb8p-module-datasheet?id=Kahvig14r1yk9) 等 | Uart2/115200 |
| raspberry pico-w | 开发中，将在 2024-11 发布 | | |


## 示例工程
tuyaopen 提供了丰富的示例工程，方便开发者快速上手，了解 tuyaopen 的使用。

```shell
$ tuyaopen
├── ai
│   └── llm_demo
├── ble
│   ├── ble_central
│   └── ble_peripher
├── get-started
│   └── sample_project
├── peripherals
│   ├── adc
│   ├── gpio
│   ├── i2c
│   ├── pwm
│   ├── spi
│   ├── timer
│   └── watchdog
├── protocols
│   ├── http_client
│   ├── mqtt
│   ├── tcp_client
│   └── tcp_server
├── system
│   ├── os_event
│   ├── os_kv
│   ├── os_mutex
│   ├── os_queue
│   ├── os_semaphore
│   ├── os_sw_timer
│   └── os_thread
└── wifi
    ├── ap
    ├── low_power
    ├── scan
    └── sta
```

每个示例工程下对应有 README.md 文件，详细介绍了示例工程的配置、编译、运行等操作。

## platform 新增与适配

tuyaopen 支持新增与适配新的 platform，具体操作请参考 [platform 新增与适配](./docs/zh/new_platform.md)。

## FAQ
1. tuyaopen 支持的 platform 通过子仓库动态下载，更新 tuyaopen 仓库不会主动更新子仓库，如遇到问题无法正常编译，请至 platform 文件夹下对应的目录下使用 `git pull` 命令更新，或删除 platform 文件夹下对应目录后再次下载。


## License
本项目的分发遵循 Apache License 版本 2.0。有关更多信息，请参见 LICENSE 文件。

## 贡献代码
如果您对 tuyaopen 感兴趣，并希望参与 tuyaopen 的开发并成为代码贡献者，请先参阅 [贡献指南](./docs/zh/contribute_guide.md)。

## 免责与责任条款

用户应明确知晓，本项目可能包含由第三方开发的子模块（submodules），这些子模块可能独立于本项目进行更新。鉴于这些子模块的更新频率不受控制，本项目无法确保这些子模块始终为最新版本。因此，用户在使用本项目时，若遇到与子模块相关的问题，建议自行根据需要进行更新或于本项目提交问题（issue）。

123

若用户决定将本项目用于商业目的，应充分认识到其中可能涉及的功能性和安全性风险。在此情况下，用户应对产品的所有功能性和安全性问题承担全部责任，应进行全面的功能和安全测试，以确保其满足特定的商业需求。本公司不对因用户使用本项目或其子模块而造成的任何直接、间接、特殊、偶然或惩罚性损害承担责任。

## 相关链接
- Arduino 版 tuyaopen：[https://github.com/tuya/arduino-tuyaopen](https://github.com/tuya/arduino-tuyaopen)
- Luanode 版 tuyaopen：[https://github.com/tuya/luanode-tuyaopen](https://github.com/tuya/luanode-tuyaopen)
