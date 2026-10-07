/*
 * util.h - hàm nhỏ dùng chung cho mọi module (bộ nhớ, thông báo,
 * chuỗi mode, số dễ đọc, in tên file).
 */
#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "options.h"

#define PROG_NAME "myls"

/* Độ dài chuỗi "drwxr-xr-x" kể cả ký tự NUL kết thúc. */
#define MODE_STRING_SIZE 11

/* malloc/realloc/strdup thoát chương trình nếu hết bộ nhớ (không segfault). */
void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *s);

/* In "myls: <what>: <strerror(errno)>" ra stderr (flush stdout trước). */
void warn_errno(const char *what);

/* Nối thư mục và tên, đảm bảo giữa hai phần có đúng một dấu '/'. */
char *path_join(const char *dir, const char *name);

/* Đọc đích của symlink vào chuỗi mới cấp phát (NULL nếu lỗi). */
char *read_link(const char *path);

/* Điền vào `out` chuỗi mode kiểu strmode, ví dụ "drwxr-xr-x". */
void mode_string(mode_t mode, char out[MODE_STRING_SIZE]);

/* Định dạng số byte dễ đọc, ví dụ "512B", "1.5K", "23M". */
void humanize(uintmax_t value, char *buf, size_t size);

/* In tên file ra stdout, tuân theo -q / -w. */
void print_name(const char *name, name_style style);

#endif /* UTIL_H */
