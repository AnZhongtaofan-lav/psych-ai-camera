# 粤嵌 GEC6818 LVGL 心理 AI 摄像头终端 — 2026-09-22 关键突破

---

## ✅ 硬件层（全部通了）

### Framebuffer
- **设备**: /dev/fb0，驱动 nxp-fb
- **尺寸**: 800×480（不是 1024×600！）
- **格式**: 32bit BGRX8888（不是 RGB565！）
- **虚拟高度**: 1440（3倍显示区），总大小 = 800×1440×4 = 4,608,000 字节 ≈ 4.4MB
- **stride**: 800×4 = 3200 字节/行
- **颜色**: BGRX8888 绿色=0x0000FF00，红色=0x00FF0000

### Backlight
- **路径**: /sys/class/backlight/pwm-backlight/brightness
- **写 255 全开**（但板子出厂默认就是开着的！只是之前 fb0 没正确写！）

### 触摸
- /dev/input/event0（evdev）

### 摄像头
- /dev/video0 ~ /dev/video6（DCMI）

### GNU/Linux
- Linux 3.4.39 armv7l，系统名 GEC6818
- 用户: root (无密码)
- 有完整 glibc: /lib/ld-linux.so.3 + /lib/libgcc_s.so + libjpeg/libpng14
- **板子上没有 gcc/tcc/cmake/python3/make**

---

## ✅ 交叉编译环境（搞定了！）

### arm-none-eabi-gcc（裸机编译器，编裸机 C + 内联汇编 syscall 能跑 Linux！）
- **安装位置**: D:\msys64\ucrt64\bin\arm-none-eabi-gcc.exe
- **版本**: GCC 16.1.0
- **使用**: 在 mingw64 终端里跑，先 `export PATH=/ucrt64/bin:$PATH`

### 编译命令（不用 newlib！不用 libc！纯内联汇编 syscall！）
```bash
arm-none-eabi-gcc -e _start -nostdlib -nostartfiles -static syscalls.c main.c -o output
```
- `-e _start`: 入口函数是 _start()
- `-nostdlib -nostartfiles`: 完全不链接任何 C 运行时
- syscalls.c: 我们自己实现的 Linux syscall 内联汇编

### Linux ARM syscall 号（r7 放 syscall 号，参数 r0-r2，swi 0x900000）
| syscall | 号 | 签名 |
|---|---|---|
| open | 5 | int open(path, flags) |
| close | 6 | int close(fd) |
| read | 3 | int read(fd, buf, n) |
| write | 4 | int write(fd, buf, n) |
| lseek | 19 | off_t lseek(fd, off, whence) |
| mmap2 | 192 | void* mmap2(addr, size, prot, flags, fd, offset4k) |
| munmap | 91 | int munmap(addr, size) |
| ioctl | 54 | int ioctl(fd, request, ...) |
| select | 142 | int select(n, rfds, wfds, efds, tv) |
| brk | 45 | void* brk(addr) |
| exit | 1 | void exit(code) |

### 板子上完整的 syscalls.c（已验证）
实现了: open/close/read/write/lseek/ioctl/select/exit + memset/memcpy/memcmp/strlen + printf(最简版: %d %x %s %c %%)

---

## ✅ 串口文件传输（唯一可用的文件传输方式！因为板子没网没U盘没USB gadget）

### 流程：Windows → 板子
1. Windows PowerShell/mingw64 里：
   ```bash
   certutil -encode input.bin output_b64.txt        # 转 base64
   sed '1d;$d' output_b64.txt > clean.txt            # 去 certutil 头尾
   ```
2. SecureCRT 板子那边：
   ```bash
   busybox cat > /tmp/output_b64                    # 准备接收
   ```
3. SecureCRT 菜单: 传输(T) → 发送 ASCII 文件 → 选 clean.txt
4. 板子 Ctrl+C 停 cat，然后：
   ```bash
   busybox base64 -d /tmp/output_b64 > /tmp/output && chmod +x /tmp/output
   ```

### 注意事项
- 115200 波特率下，10KB 文件传几秒就完
- **必须用 ASCII 上传**（二进制直接传会出乱码）
- certutil 是 Windows 自带的 base64 工具，不用额外装

---

## ✅ 已验证能跑的程序

### 1. hello3（write syscall + printf）
- 纯 write syscall 写控制台
- 板子输出 "Hello from GEC6818 LVGL board!"

### 2. fb_write32（全绿 framebuffer）
- open /dev/fb0 + 栈上每行写 3200 字节绿色（BGRX8888 绿色=0x0000FF00）
- 480 次 write 写满 800×480
- **板子屏幕全绿成功！** ✅

### 3. fb_test3（syscalls.c + printf + open + ioctl）
- 验证 syscalls 组合能编能跑
- ioctl 返回 r=-14（参数结构 offset 错），但 open/write/printf 全通

---

## 🎯 LVGL 编译（进行中）

### LVGL 源码
- **位置**: d:\psych-architect\psych\lvgl_sim\lvgl\src\
- **版本**: v9.x (lv_conf.h 里 COLOR_DEPTH 32)
- **LVGL 用 STDLIB_BUILTIN**: malloc/strlen/sprintf 都是 LVGL 自己实现的！只需要我们提供 syscalls！

### 关键配置（lv_conf.h 要确认）
- ✅ LV_COLOR_DEPTH 32
- ✅ LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
- ✅ LV_USE_STDLIB_STRING LV_STDLIB_BUILTIN
- ✅ LV_USE_STDLIB_SPRINTF LV_STDLIB_BUILTIN
- ❌ LV_USE_FS_STDIO 要关（=0，因为没 DIR dirent）
- ❌ main_board.c 里的 linux 头要去掉（arm-none-eabi 没有！），自己写 shim

### main_board.c 需要改的
1. **分辨率**: SCREEN_W 1024→800, SCREEN_H 600→480
2. **颜色格式**: fbdev_flush_cb 里的 R5G6B5 → BGRX8888
3. **头文件**: `<linux/fb.h>` `<linux/videodev2.h>` `<linux/input.h>` `<sys/mman.h>` `<sys/ioctl.h>` `<sys/select.h>` 全部不能用！自己 shim 定义 struct fb_var_screeninfo / input_event / v4l2_format 等
4. **mmap**: mmap2 syscall 我们还没写！要补 mmap2 和 munmap！
5. **select**: select syscall 我们已经有了 ✅

### 编译命令（循环 226 个 LVGL .c）
```bash
cd /d/psych-architect/psych/lvgl_sim
for f in $(find lvgl/src -name "*.c" ! -path "*/libs/fsdrv/*" ! -path "*/osal/*" ! -path "*/misc/*" ! -path "*/others/*" ! -path "*/drivers/sdl/*" ! -path "*/drivers/x11/*" -type f); do
    arm-none-eabi-gcc -c $f -o /tmp/$(basename $f).o \
        -DLV_USE_LIBPNG=0 -DLV_USE_FREETYPE=0 -DLV_USE_OS=0 \
        -DLV_USE_FS_POSIX=0 -DLV_USE_FS_STDIO=0 -DLV_USE_FS_WIN32=0 \
        -I. -Ilvgl -Ilvgl/src -O2
done
# 然后链接 syscalls.o + main_board.o + 所有 lvgl *.o
```

### 要补的 syscall
- mmap2 (SYS_mmap2 = 192) — main_board.c 用了！
- munmap (SYS_munmap = 91) — main_board.c 用了！

---

## ⚠️ 坑记录

| 坑 | 解 |
|---|---|
| arm-none-eabi-gcc 能编 Linux ARM ELF 吗？ | 能！不用 arm-linux-gnueabi！用 `-e _start -nostdlib -nostartfiles -static` + 内联汇编 swi 0x900000！ |
| newlib 的 _open_r/_times_r 等签名对不上 | 放弃 newlib！不用它！纯内联汇编！ |
| SecureCRT ASCII 上传旧 cat 接收 Ctrl+C 停不掉 | 等 SecureCRT 底部显示"就绪"再按！或者关掉重开 SecureCRT！ |
| 板子屏幕一半绿一半黑 | 因为之前按 16bit RGB565 算，实际是 32bit BGRX8888！只写了一半字节！ |
| framebuffer 格式猜 RGB565 错 | fbset 里 `geometry 800 480 800 1440 32` + `rgba 8/16,8/8,8/0,8/24` → BGRX8888 32bit！ |
| kernel.org 下的 gcc 是 x86_64 host 版 | 不能在板子跑！是 Windows/Linux PC 上跑的交叉编译器！ELF class=x86_64 → board 上 wrong ELF class！ |
| 板子 eth0 ping 不通 Windows | 网段不同：板子 192.168.168.33 vs Windows 192.168.240.176！没路由器桥接！ |
| 板子 USB gadget (g_ether) | modprobe g_ether 找不到！内核没编！ |
| mingw64 ucrt64 arm-none-eabi-gcc 不在 PATH | mingw64 里先 `export PATH=/ucrt64/bin:$PATH` |
| certutil 编出的 base64 有头尾 | sed '1d;$d' 去掉！ |

---

## 🔑 关键文件路径

| 文件 | 路径 | 作用 |
|---|---|---|
| syscalls.c | /tmp/syscalls.c (mingw64 里) | 迷你 C runtime，内联汇编 syscall |
| fb_write32 | /tmp/fb_write32 | 编成功的 framebuffer 全绿验证程序 |
| hello3 | /tmp/hello3 | 编成功的 write+printf 验证程序 |
| main_board.c | D:\psych-architect\psych\lvgl_sim\main_board.c | 板子驱动入口（fbdev+evdev+V4L2） |
| Makefile.arm | D:\psych-architect\psych\lvgl_sim\Makefile.arm | 原 Makefile（CROSS 指到 kernel.org 那个 gcc，不对） |
| lv_conf.h | D:\psych-architect\psych\lvgl_sim\lv_conf.h | LVGL 配置（COLOR_DEPTH=32） |
| LVGL 源码 | D:\psych-architect\psych\lvgl_sim\lvgl\src\ | v9.x 源码 |
| arm-none-eabi-gcc | D:\msys64\ucrt64\bin\arm-none-eabi-gcc.exe | 16.1.0 交叉编译器 |

---

## 👆 板子硬件速查

```
开发板:     粤嵌 GEC6818 (S5PV210 Cortex-A8)
屏幕:       800×480 7寸电阻触摸 (注意 fbset 报告 800x480，不是文档说的 1024x600！)
格式:       BGRX8888 32bit (不是 RGB565！)
串口:       COM8 / 115200 / 8N1 (DEBUG口 USB→串口线)
登录:       root (无密码)
Framebuffer:/dev/fb0 (nxp-fb 驱动)
触摸:       /dev/input/event0 (evdev)
摄像头:     /dev/video0 (DCMI)
网卡:       eth0 (未插网线)
SSH:        :22 LISTEN (但 eth0 没通)
USB Host:   有 ×4
SD卡槽:     有
OTG烧录口:  有 (但 g_ether gadget 内核没编)
IP:         192.168.168.33 (eth0，未插网线)
```
[session_id: 6ab24f28e001dc3ce9380891 | topic_summary_time: 2026-09-22 19:08:40]用户继续粤嵌GEC6818 LVGL心理AI摄像头终端项目，尝试解决LVGL编译和运行问题。之前的编译命令因使用`-c`选项同时处理多个.c文件导致GCC报错，已通过`build_board.sh`脚本中的for循环逐个编译211个LVGL源文件解决。传输`gec6818_app.b64`到开发板后，执行时出现段错误，通过`dmesg`定位到可能与`ioctl`、`mmap`或`__aeabi_uidiv`相关。随后开发了`fb_probe`探测程序，发现问题根源是自定义`memset`被GCC内建函数替换导致无限递归爆栈，添加`-fno-builtin`选项修复。最终重新编译完整LVGL程序，包含r4/r5寄存器保护、`-fno-builtin`和补齐字符串函数等修复，准备再次通过串口传输`gec6818_app_b64.txt`到开发板运行。
[session_id: 6ab24f28e001dc3ce9380891 | topic_summary_time: 2026-09-22 19:39:31]User encountered a black screen issue with the GEC6818 development board when running the LVGL application. The problem was traced to an incorrect framebuffer buffer size calculation in LVGL v9, where `lv_color_t` was treated as 3 bytes instead of XRGB8888's 4 bytes, causing an assertion failure at `lv_display.c:420` (stride * h <= buf_size). The buffer size was corrected to 1,536,000 bytes (800x480x32bpp), resolving the assertion. After retransmitting the updated `gec6818_app` (552964 bytes), the serial output showed `3 screens ready`, and the display showed the CHAT interface with a dark background, cyan title, and bottom navigation buttons. A subsequent `fb_lprobe` utility was used to verify framebuffer orientation, revealing the board was physically rotated 90 degrees counterclockwise relative to the framebuffer coordinates. Rotating the board corrected the display alignment, confirming the issue was due to physical orientation rather than software rotation.
[session_id: 6ab24f28e001dc3ce9380891 | topic_summary_time: 2026-09-22 20:10:38]User is working on a project requiring camera image display, photo/video capture and viewing, LVGL interface design, and AI model interaction. Current status: LVGL interface (3 pages with buttons and navigation) is complete, but camera functions are not implemented (simulated gradient instead of real capture, capture/record buttons only print messages, media viewing page shows static text). The GEC6818 board has no recognized camera hardware (scanned video nodes are internal image processors, not capture devices). User lacks a USB camera and has no router for network connectivity; attempts to use phone USB tethering are pending driver verification. Next steps depend on obtaining a USB camera and resolving network access (via router, USB Ethernet adapter, or phone tethering) to implement remaining features.
[session_id: 6ab24f28e001dc3ce9380891 | topic_summary_time: 2026-09-22 21:31:25]用户在GEC6818开发板上部署了包含CHAT、PHOTOS、VIDEO三个页面的LVGL应用。首先解决了固件传输和显示驱动问题，通过base64编码传输gec6818_app.b64文件并解码执行，确认800x480分辨率和触摸设备/dev/input/event0正常工作。接着修复触摸无响应问题，通过touch_probe程序收集触摸事件数据，调整坐标映射（1024x600缩放到800x480）和设置非阻塞读取，使页面切换和屏幕键盘输入功能正常。随后传输测试媒体文件（p1.jpg、p2.jpg、p3.jpg、v1.mjpeg）到/tmp/media目录，验证相册缩略图显示、全屏查看及视频播放功能。最后指导用户在siliconflow.cn注册获取免费API Key，配置agent_relay.py脚本，通过串口转发实现开发板与大模型的实时对话，完成智能体功能部署。
