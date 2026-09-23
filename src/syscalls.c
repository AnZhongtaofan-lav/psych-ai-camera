#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#define SYS_open 5
#define SYS_close 6
#define SYS_read 3
#define SYS_write 4
#define SYS_lseek 19
#define SYS_mmap2 192
#define SYS_munmap 91
#define SYS_ioctl 54
#define SYS_select 142
#define SYS_brk 45
#define SYS_exit 1

static long _swi(int n, long a1, long a2, long a3, long a4) {
    register int r0 __asm__("r0") = a1;
    register int r1 __asm__("r1") = a2;
    register int r2 __asm__("r2") = a3;
    register int r3 __asm__("r3") = a4;
    register int r7 __asm__("r7") = n;
    __asm__ volatile("swi 0x900000" : "+r"(r0) : "r"(r1),"r"(r2),"r"(r3),"r"(r7) : "memory");
    return r0;
}

int _open(const char *p, int f, ...) { return (int)_swi(SYS_open, (long)p, f, 0, 0); }
int _close(int fd) { return (int)_swi(SYS_close, fd, 0, 0, 0); }
int _read(int fd, void *b, int n) { return (int)_swi(SYS_read, fd, (long)b, n, 0); }
int _write(int fd, const void *b, int n) { return (int)_swi(SYS_write, fd, (long)b, n, 0); }
off_t _lseek(int fd, off_t o, int w) { return (off_t)_swi(SYS_lseek, fd, o, w, 0); }
int _ioctl(int fd, unsigned req, ...) { return (int)_swi(SYS_ioctl, fd, req, 0, 0); }
int _select(int n, void *rfds, void *wfds, void *efds, void *tv) {
    return (int)_swi(SYS_select, n, (long)rfds, (long)wfds, (long)efds);
}
void _exit(int st) { _swi(SYS_exit, st, 0, 0, 0); while(1); }

void *_memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char*)s;
    while(n--) *p++ = (unsigned char)c;
    return s;
}
void *_memcpy(void *d, const void *s, size_t n) {
    unsigned char *dd = (unsigned char*)d;
    const unsigned char *ss = (const unsigned char*)s;
    while(n--) *dd++ = *ss++;
    return d;
}
int _memcmp(const void *a, const void *b, size_t n) {
    const unsigned char *aa = (const unsigned char*)a;
    const unsigned char *bb = (const unsigned char*)b;
    while(n--) { if(*aa != *bb) return *aa - *bb; aa++; bb++; }
    return 0;
}
size_t _strlen(const char *s) { size_t n=0; while(s[n]) n++; return n; }

static void _puts(const char *s) { while(*s) _write(1, s++, 1); }
static void _print_int(int n) { char b[16]; int i=0, neg=0; if(n<0){neg=1;n=-n;} if(n==0) b[i++]='0'; while(n>0){b[i++]='0'+(n%10);n/=10;} if(neg) _puts("-"); while(i>0) _write(1,&b[--i],1); }
static void _print_hex(unsigned n) { const char h[]="0123456789ABCDEF"; int i=0; char b[12]; if(n==0) b[i++]='0'; while(n>0){b[i++]=h[n&0xF];n>>=4;} while(i>0) _write(1,&b[--i],1); }

#include <stdarg.h>
int _printf(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    while(*fmt) {
        if(*fmt != '%') { _write(1, fmt, 1); fmt++; continue; }
        fmt++;
        switch(*fmt++) {
        case 'd': _print_int(va_arg(ap, int)); break;
        case 'x': case 'X': _print_hex(va_arg(ap, unsigned)); break;
        case 's': _puts(va_arg(ap, const char*)); break;
        case 'c': _write(1, &(char){(char)va_arg(ap, int)}, 1); break;
        case '%': _write(1, "%", 1); break;
        }
    }
    va_end(ap); return 0;
}
