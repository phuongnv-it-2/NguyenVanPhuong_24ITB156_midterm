/*
 * format.c - định dạng đầu ra: inode, số block, dạng dài (-l),
 * ký hiệu -F và căn cột.
 */
#include <grp.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h> /* major(), minor() trên NetBSD */
#include <time.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/sysmacros.h> /* major(), minor() */
#endif

#include "format.h"
#include "util.h"

#define FIELD_BUFFER_SIZE 256
#define BYTES_PER_BLOCK 512
#define SECONDS_PER_DAY (24 * 60 * 60)
/* POSIX coi "gần đây" là trong vòng 6 tháng; 182.5 ngày là xấp xỉ thường dùng.
 * File cũ hơn (hoặc có thời gian ở tương lai) sẽ hiện năm thay cho giờ. */
#define RECENT_TIME_WINDOW ((time_t)(182.5 * SECONDS_PER_DAY))

/* Độ rộng từng cột của một lần liệt kê để các cột thẳng hàng. */
struct column_widths {
    size_t inode, blocks, link_count, owner, group, size;
};

/* ---- hàm hỗ trợ từng trường ------------------------------------------ */

/* Đổi số block 512 byte sang đơn vị do -h/-k/BLOCKSIZE chọn. */
static void format_blocks(uintmax_t blocks512, const struct ls_options *opt,
                          char *buf, size_t size)
{
    uintmax_t bytes = blocks512 * BYTES_PER_BLOCK;
    uintmax_t unit;

    if (opt->human) {
        humanize(bytes, buf, size);
        return;
    }
    unit = opt->kilo ? 1024 : opt->block_unit;
    /* đơn vị lẻ được làm tròn lên theo man page */
    snprintf(buf, size, "%ju", (bytes + unit - 1) / unit);
}

static void owner_string(uid_t uid, int numeric, char *buf, size_t size)
{
    static uid_t last_uid;
    static char last_name[FIELD_BUFFER_SIZE];
    static int cached;

    if (!numeric) {
        if (!cached || last_uid != uid) {
            struct passwd *pw = getpwuid(uid);

            if (pw != NULL) {
                snprintf(last_name, sizeof last_name, "%s", pw->pw_name);
                last_uid = uid;
                cached = 1;
            } else {
                cached = 0;
            }
        }
        if (cached) {
            snprintf(buf, size, "%s", last_name);
            return;
        }
    }
    /* user không tồn tại hoặc dùng -n thì in số */
    snprintf(buf, size, "%ju", (uintmax_t)uid);
}

static void group_string(gid_t gid, int numeric, char *buf, size_t size)
{
    static gid_t last_gid;
    static char last_name[FIELD_BUFFER_SIZE];
    static int cached;

    if (!numeric) {
        if (!cached || last_gid != gid) {
            struct group *gr = getgrgid(gid);

            if (gr != NULL) {
                snprintf(last_name, sizeof last_name, "%s", gr->gr_name);
                last_gid = gid;
                cached = 1;
            } else {
                cached = 0;
            }
        }
        if (cached) {
            snprintf(buf, size, "%s", last_name);
            return;
        }
    }
    snprintf(buf, size, "%ju", (uintmax_t)gid);
}

/* Cột kích thước: số byte, dạng dễ đọc, hoặc "major, minor" với file thiết bị. */
static void size_string(const struct stat *st, const struct ls_options *opt,
                        char *buf, size_t size)
{
    if (S_ISCHR(st->st_mode) || S_ISBLK(st->st_mode))
        snprintf(buf, size, "%ju, %ju", (uintmax_t)major(st->st_rdev),
                 (uintmax_t)minor(st->st_rdev));
    else if (opt->human)
        humanize((uintmax_t)st->st_size, buf, size);
    else
        snprintf(buf, size, "%ju", (uintmax_t)st->st_size);
}

/* "Mon dd HH:MM" cho file gần đây, "Mon dd  YYYY" cho file cũ. */
static void time_string(const struct stat *st, const struct ls_options *opt,
                        time_t now, char *buf, size_t size)
{
    struct timespec ts = entry_time(st, opt->time_sel);
    time_t t = ts.tv_sec;
    struct tm tm;
    const char *fmt;

    if (localtime_r(&t, &tm) == NULL) {
        snprintf(buf, size, "?");
        return;
    }
    fmt = (t > now || t < now - RECENT_TIME_WINDOW) ? "%b %e  %Y" : "%b %e %H:%M";
    if (strftime(buf, size, fmt, &tm) == 0)
        snprintf(buf, size, "?");
}

/* Ký tự -F thêm vào sau tên, hoặc 0 nếu không có. */
static char classify_char(mode_t mode)
{
    if (S_ISDIR(mode))
        return '/';
    if (S_ISLNK(mode))
        return '@';
#ifdef S_IFWHT
    if ((mode & S_IFMT) == S_IFWHT)
        return '%';
#endif
    if (S_ISSOCK(mode))
        return '=';
    if (S_ISFIFO(mode))
        return '|';
    if (mode & (S_IXUSR | S_IXGRP | S_IXOTH))
        return '*';
    return 0;
}

static void widen_to_fit(size_t *width, const char *text)
{
    size_t length = strlen(text);

    if (length > *width)
        *width = length;
}

/* Phải đo độ rộng mọi cột trước khi in, vì dòng đầu cần biết giá trị lớn nhất. */
static struct column_widths measure_columns(const struct entry_list *list,
                                            const struct ls_options *opt)
{
    struct column_widths widths = {0, 0, 0, 0, 0, 0};
    char text[FIELD_BUFFER_SIZE];
    size_t i;

    for (i = 0; i < list->len; i++) {
        const struct stat *file_stat = &list->items[i].st;

        if (opt->show_inode) {
            snprintf(text, sizeof text, "%ju", (uintmax_t)file_stat->st_ino);
            widen_to_fit(&widths.inode, text);
        }
        if (opt->show_blocks) {
            format_blocks((uintmax_t)file_stat->st_blocks, opt, text,
                          sizeof text);
            widen_to_fit(&widths.blocks, text);
        }
        if (!opt->long_format)
            continue;

        snprintf(text, sizeof text, "%ju", (uintmax_t)file_stat->st_nlink);
        widen_to_fit(&widths.link_count, text);
        owner_string(file_stat->st_uid, opt->numeric_ids, text, sizeof text);
        widen_to_fit(&widths.owner, text);
        group_string(file_stat->st_gid, opt->numeric_ids, text, sizeof text);
        widen_to_fit(&widths.group, text);
        size_string(file_stat, opt, text, sizeof text);
        widen_to_fit(&widths.size, text);
    }
    return widths;
}

/* In các cột của dạng -l: mode, số link, owner, group, kích thước, ngày. */
static void print_long_columns(const struct stat *file_stat,
                               const struct ls_options *opt,
                               const struct column_widths *widths,
                               time_t now)
{
    char mode[MODE_STRING_SIZE];
    char owner[FIELD_BUFFER_SIZE], group[FIELD_BUFFER_SIZE];
    char size[FIELD_BUFFER_SIZE], timestamp[FIELD_BUFFER_SIZE];

    mode_string(file_stat->st_mode, mode);
    owner_string(file_stat->st_uid, opt->numeric_ids, owner, sizeof owner);
    group_string(file_stat->st_gid, opt->numeric_ids, group, sizeof group);
    size_string(file_stat, opt, size, sizeof size);
    time_string(file_stat, opt, now, timestamp, sizeof timestamp);
    /* Giống ls của NetBSD: hai dấu cách sau mode, owner và group. */
    printf("%s  %*ju %-*s  %-*s  %*s %s ", mode, (int)widths->link_count,
           (uintmax_t)file_stat->st_nlink, (int)widths->owner, owner,
           (int)widths->group, group, (int)widths->size, size, timestamp);
}

/* In " -> đích". Trả về 1 nếu không đọc được đích của symlink. */
static int print_symlink_target(const struct entry *entry,
                                const struct ls_options *opt)
{
    char *target = read_link(entry->path);

    if (target == NULL) {
        warn_errno(entry->path);
        return 1;
    }
    fputs(" -> ", stdout);
    print_name(target, opt->name_mode);
    free(target);
    return 0;
}

/* Trả về 1 nếu entry không in được đầy đủ. */
static int print_entry(const struct entry *entry, const struct ls_options *opt,
                       const struct column_widths *widths, time_t now)
{
    const struct stat *file_stat = &entry->st;
    char text[FIELD_BUFFER_SIZE];
    char classifier;
    int has_error = 0;

    if (opt->show_inode)
        printf("%*ju ", (int)widths->inode, (uintmax_t)file_stat->st_ino);
    if (opt->show_blocks) {
        format_blocks((uintmax_t)file_stat->st_blocks, opt, text, sizeof text);
        printf("%*s ", (int)widths->blocks, text);
    }
    if (opt->long_format)
        print_long_columns(file_stat, opt, widths, now);

    print_name(entry->name, opt->name_mode);
    classifier = opt->classify ? classify_char(file_stat->st_mode) : 0;
    if (classifier)
        putchar(classifier);
    if (opt->long_format && S_ISLNK(file_stat->st_mode))
        has_error = print_symlink_target(entry, opt);
    putchar('\n');
    return has_error;
}

/* ---- hàm công khai ---------------------------------------------------- */

void print_total(const struct entry_list *list, const struct ls_options *opt)
{
    uintmax_t total_blocks = 0;
    char text[FIELD_BUFFER_SIZE];
    size_t i;

    for (i = 0; i < list->len; i++)
        total_blocks += (uintmax_t)list->items[i].st.st_blocks;
    format_blocks(total_blocks, opt, text, sizeof text);
    printf("total %s\n", text);
}

int print_entries(const struct entry_list *list, const struct ls_options *opt)
{
    struct column_widths widths = measure_columns(list, opt);
    time_t now = time(NULL);
    int has_error = 0;
    size_t i;

    for (i = 0; i < list->len; i++) {
        if (print_entry(&list->items[i], opt, &widths, now))
            has_error = 1;
    }
    return has_error;
}
