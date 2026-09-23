void _start(void) {
    register int r0 __asm__("r0"), r1 __asm__("r1"), r2 __asm__("r2"), r7 __asm__("r7");
    register int r3 __asm__("r3"), r4 __asm__("r4"), r5 __asm__("r5");

    r0 = (int)"/dev/fb0"; r1 = 2; r2 = 0; r7 = 5;
    __asm__ volatile("swi 0x900000" : "+r"(r0) : "r"(r1),"r"(r2),"r"(r7));
    int fd = r0;

    // 栈上准备一行绿色（BGRX8888），800个int = 3200字节
    unsigned int linebuf[800];
    for (r3 = 0; r3 < 800; r3++) ((unsigned int*)linebuf)[r3] = 0x0000FF00;

    for (r4 = 0; r4 < 480; r4++) {
        r0 = fd; r1 = (int)linebuf; r2 = 800 * 4; r7 = 4;
        __asm__ volatile("swi 0x900000" : "+r"(r0) : "r"(r1),"r"(r2),"r"(r7));
    }

    r0 = 0; r7 = 1;
    __asm__ volatile("swi 0x900000" : : "r"(r0),"r"(r7));
    while(1);
}
