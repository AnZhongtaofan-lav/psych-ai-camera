# 嵌入式 Linux 智能摄像头终端

## 硬件
粤嵌 GEC6818 (ARM Cortex-A8 S5PV210)，Linux 3.4.39

## 技术栈
- Linux / LVGL v9.x
- arm-none-eabi-gcc 交叉编译
- C 内联汇编 syscall (swi 0x900000)
- V4L2 / evdev / framebuffer
- Python FastAPI + MySQL

## 特色
- **裸机工具链 + 内联汇编 syscall**：绕开完整 libc 编 Linux ELF
- **无网无U盘场景**：base64 + 串口 ASCII 上传方案
- **framebuffer 格式踩坑**：文档写 RGB565 实际是 BGRX8888 32bit 800x480

## 编译
```bash
export PATH=/ucrt64/bin:$PATH
cd /tmp
arm-none-eabi-gcc -e _start -nostdlib -nostartfiles syscalls.c fb_write32.c -o fb_write32
```

## 部署
Windows:
```bash
certutil -encode fb_write32 app_b64.txt
sed '1d;$d' app_b64.txt > clean.txt
# SecureCRT 发送 ASCII → clean.txt
```

板子:
```bash
busybox base64 -d clean.txt > app && chmod +x app && ./app
```

## 代码
| 文件 | 说明 |
|---|---|
| src/syscalls.c | 内联汇编 Linux syscall 运行时（open/close/read/write/mmap/ioctl/select + printf/memset） |
| src/fb_write32.c | framebuffer 验证程序（全屏绿色） |
| src/main_board.c | 板子 LVGL 驱动入口（fbdev+evdev+V4L2） |

## 详细开发日志
见 [docs/topics.md](docs/topics.md) — 200+ 行硬件参数、syscall 映射、踩坑复盘文档
