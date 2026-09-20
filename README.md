# C542RC USB Audio Loopback

这是一个运行在 NUCLEO-C542RC 上的 USB 音频环回固件。主机发送到播放端点的数据会被原样送回录音端点，可用于检查 USB Audio 双向等时传输、PMA 双缓冲和主机音频链路。

## 配置

| 项目 | 当前值 |
| --- | --- |
| 开发板 | NUCLEO-C542RC |
| MCU | STM32C542RCT6，Cortex-M33，144 MHz |
| USB 模式 | USB Full Speed Device |
| USB 协议栈 | USBX 6.5.0，No OS |
| 音频类别 | USB Audio Class 1.0 |
| 音频格式 | 48 kHz、双声道、16-bit PCM |
| USB 帧 | 1 ms，192 字节 |
| 播放端点 | EP1 OUT，地址 `0x01`，双缓冲、同步等时 |
| 录音端点 | EP2 IN，地址 `0x82`，双缓冲、等时 |
| USB 时钟 | 24 MHz HSE → 144 MHz PSI → PSIDIV3 48 MHz |

设备在主机上的产品名称为 `STM32 USB Audio Loopback`。

## 构建

需要以下工具：

- CMake 3.30 或更高版本；
- Ninja；
- GNU Arm Embedded Toolchain 14.3.1，`arm-none-eabi-gcc` 需要在 `PATH` 中。

在仓库根目录执行：

```powershell
cd C542RC_AudioLoopback_cmake
cmake --preset debug_GCC_NUCLEO-C542RC
cmake --build --preset debug_GCC_NUCLEO-C542RC
```

完整清理并重新构建：

```powershell
cmake --build --preset debug_GCC_NUCLEO-C542RC --clean-first
```

生成的固件位于：

```text
C542RC_AudioLoopback_cmake/build/debug_GCC_NUCLEO-C542RC/C542RC_AudioLoopback.elf
```

## 预期验证方法

1. 将 ELF 固件烧录到 NUCLEO-C542RC。
2. 把开发板***下方***的 USB 接口连接到主机。
3. 确认主机识别出名为 `STM32 USB Audio Loopback` 的播放和录音设备。
4. 将播放端和录音端都设为 48 kHz、双声道、16-bit。
5. 播放已知音频，同时从该设备录音，对比发送数据和录音数据。

以上是根据代码得到的预期行为，不代表已经完成实机验证。

## PMA 与 PCD 配置

当前 PMA 分配如下：

| 端点 | 缓冲模式 | PMA 地址 |
| --- | --- | --- |
| EP0 IN | 单缓冲 | `0x020` |
| EP0 OUT | 单缓冲 | `0x060` |
| EP2 IN | 双缓冲 | `0x0A0`、`0x160` |
| EP1 OUT | 双缓冲 | `0x220`、`0x2E0` |

PCD 编译配置必须同时启用：

```c
#define USE_HAL_PCD_USB_DOUBLE_BUFFER 1U
#define USE_HAL_PCD_USB_EP_TYPE_ISOC  1U
```

USB SOF 已启用，USB 中断优先级为 0。

## 目录

```text
.
├── C542RC_AudioLoopback.ioc2       CubeMX 工程
├── C542RC_AudioLoopback_cmake/     CMake 固件工程
│   ├── generated/hal/              时钟、USB DRD、NVIC 等生成代码
│   ├── generated/middleware/       USBX 初始化和音频应用
│   ├── middleware/usbx/            USBX 与 STM32 适配层
│   ├── stm32c5xx_drivers/          STM32C5 HAL 驱动
│   ├── main.c                      程序入口
│   └── CMakePresets.json           Debug 构建预设
└── README.md
```

主要实现文件：

- [`mx_usbx_device_audio.c`](C542RC_AudioLoopback_cmake/generated/middleware/mx_usbx_device_audio.c)：双向音频流、环形缓冲区和静音控制；
- [`mx_usbx_device.c`](C542RC_AudioLoopback_cmake/generated/middleware/mx_usbx_device.c)：USBX Device 和 Audio Class 初始化；
- [`ux_stm32_device_descriptors_config.h`](C542RC_AudioLoopback_cmake/generated/middleware/ux_stm32_device_descriptors_config.h)：音频格式、端点和设备字符串；
- [`ux_stm32_device_descriptors.c`](C542RC_AudioLoopback_cmake/middleware/usbx/interfaces/usbx_stm32_device_descriptors/ux_stm32_device_descriptors.c)：UAC1 描述符构建；
- [`ux_dcd_stm32_callback.c`](C542RC_AudioLoopback_cmake/middleware/usbx/interfaces/usbx_stm32_device_controllers/ux_dcd_stm32_callback.c)：等时 OUT 每包长度处理和中断回调；
- [`mx_usb_drd_fs.c`](C542RC_AudioLoopback_cmake/generated/hal/mx_usb_drd_fs.c)：PCD、PMA、SOF 和中断配置；
- [`mx_rcc.c`](C542RC_AudioLoopback_cmake/generated/hal/mx_rcc.c)：系统时钟和 USB 48 MHz 时钟配置。

## 当前限制

- 只支持 USB Full Speed；
- 音频格式固定为 48 kHz、双声道、16-bit PCM；
- 不支持运行时切换采样率；
- 没有异步反馈端点；

## 许可

本项目代码采用 [MIT License](LICENSE)。仓库内的 USBX、CMSIS、STM32 HAL 和生成代码仍遵循各自目录中的许可证文件；这些第三方许可证不因根目录的 MIT License 而改变。
