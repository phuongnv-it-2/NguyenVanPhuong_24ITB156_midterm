/*
 * util.c - hàm dùng chung: cấp phát bộ nhớ, thông báo lỗi, xử lý đường dẫn,
 * chuỗi mode, kích thước dễ đọc và in tên file.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

#include "util.h"

void *xmalloc(size_t size)
{
    void *p = malloc(size ? size : 1);

    if (p == NULL) {
        fprintf(stderr, "%s: out of memory\n", PROG_NAME);
        exit(EXIT_FAILURE);
    }
    return p;
}

void *xrealloc(void *ptr, size_t size)
{
    void *p = realloc(ptr, size ? size : 1);

    if (p == NULL) {
        fprintf(stderr, "%s: out of memory\n", PROG_NAME);
        exit(EXIT_FAILURE);
    }
    return p;
}

char *xstrdup(const char *s)
{
    size_t len = strlen(s) + 1;

    return memcpy(xmalloc(len), s, len);
}

void warn_errno(const char *what)
{
    int saved = errno;

    /* Flush stdout để thông báo lỗi xuất hiện đúng thứ tự thực tế. */
    fflush(stdout);
    fprintf(stderr, "%s: %s: %s\n", PROG_NAME, what, strerror(saved));
}

char *path_join(const char *dir, const char *name)
{
    size_t dlen = strlen(dir);
    int need_slash = (dlen > 0 && dir[dlen - 1] != '/');
    char *out = xmalloc(dlen + need_slash + strlen(name) + 1);

    memcpy(out, dir, dlen);
    if (need_slash)
        out[dlen++] = '/';
    strcpy(out + dlen, name);
    return out;
}

char *read_link(const char *path)
{
    size_t cap = 128;

    for (;;) {
        char *buf = xmalloc(cap);
        ssize_t n = readlink(path, buf, cap);

        if (n < 0) {
            free(buf);
            return NULL;
        }
        if ((size_t)n < cap) { /* vừa bộ đệm: readlink không tự thêm ký tự NUL */
            buf[n] = '\0';
            return buf;
        }
        free(buf);             /* có thể bị cắt: thử lại với bộ đệm lớn hơn */
        cap *= 2;
    }
}

void mode_string(mode_t mode, char out[MODE_STRING_SIZE])
{
    /* Ký tự loại file. 'a'/'A' (trạng thái archive) cần cờ file của BSD,
     * Linux không có nên không hiển thị. */
    switch (mode & S_IFMT) {
    case S_IFREG:  out[0] = '-'; break;
    case S_IFDIR:  out[0] = 'd'; break;
    case S_IFLNK:  out[0] = 'l'; break;
    case S_IFBLK:  out[0] = 'b'; break;
    case S_IFCHR:  out[0] = 'c'; break;
    case S_IFIFO:  out[0] = 'p'; break;
    case S_IFSOCK: out[0] = 's'; break;
#ifdef S_IFWHT
    case S_IFWHT:  out[0] = 'w'; break;
#endif
    default:       out[0] = '?'; break;
    }

    /* Quyền của owner */
    out[1] = (mode & S_IRUSR) ? 'r' : '-';
    out[2] = (mode & S_IWUSR) ? 'w' : '-';
    if (mode & S_ISUID)
        out[3] = (mode & S_IXUSR) ? 's' : 'S';
    else
        out[3] = (mode & S_IXUSR) ? 'x' : '-';

    /* Quyền của group */
    out[4] = (mode & S_IRGRP) ? 'r' : '-';
    out[5] = (mode & S_IWGRP) ? 'w' : '-';
    if (mode & S_ISGID)
        out[6] = (mode & S_IXGRP) ? 's' : 'S';
    else
        out[6] = (mode & S_IXGRP) ? 'x' : '-';

    /* Quyền của other; bit sticky hiện ở đây dưới dạng t / T */
    out[7] = (mode & S_IROTH) ? 'r' : '-';
    out[8] = (mode & S_IWOTH) ? 'w' : '-';
    if (mode & S_ISVTX)
        out[9] = (mode & S_IXOTH) ? 't' : 'T';
    else
        out[9] = (mode & S_IXOTH) ? 'x' : '-';
    out[MODE_STRING_SIZE - 1] = '\0';
}

void humanize(uintmax_t value, char *buf, size_t size)
{
    static const char units[] = "BKMGTPE";
    double d = (double)value;
    int i = 0;

    if (value < 1024) {
        snprintf(buf, size, "%juB", value);
        return;
    }
    while (d >= 1024.0 && i < 6) {
        d /= 1024.0;
        i++;
    }
    if (d < 9.95) {
        snprintf(buf, size, "%.1f%c", d, units[i]);
    } else {
        uintmax_t r = (uintmax_t)(d + 0.5);

        if (r >= 1024 && i < 6)      /* ví dụ 1023.9K phải hiện là 1.0M */
            snprintf(buf, size, "1.0%c", units[i + 1]);
        else
            snprintf(buf, size, "%ju%c", r, units[i]);
    }
}

void print_name(const char *name, name_style style)
{
    mbstate_t ps;
    const char *p = name;
    size_t left = strlen(name);

    if (style == NAME_RAW) {
        fputs(name, stdout);
        return;
    }

    /* -q: giải mã ký tự nhiều byte; ký tự không in được thành '?'. */
    memset(&ps, 0, sizeof ps);
    while (left > 0) {
        wchar_t wc;
        size_t n = mbrtowc(&wc, p, left, &ps);

        if (n == (size_t)-1 || n == (size_t)-2) { /* byte sai hoặc chuỗi bị cắt */
            putchar('?');
            memset(&ps, 0, sizeof ps);
            p++;
            left--;
            continue;
        }
        if (n == 0)
            n = 1;
        if (iswprint((wint_t)wc))
            fwrite(p, 1, n, stdout);
        else
            putchar('?');
        p += n;
        left -= n;
    }
}
