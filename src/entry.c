/*
 * entry.c - danh sách entry và chọn loại mốc thời gian.
 */
#include <stdlib.h>
#include <string.h>

#include "entry.h"
#include "util.h"

/* Trên macOS các trường thời gian (nano giây) có tên khác với Linux. */
#if defined(__APPLE__)
#define ST_ATIM(s) ((s)->st_atimespec)
#define ST_MTIM(s) ((s)->st_mtimespec)
#define ST_CTIM(s) ((s)->st_ctimespec)
#else
#define ST_ATIM(s) ((s)->st_atim)
#define ST_MTIM(s) ((s)->st_mtim)
#define ST_CTIM(s) ((s)->st_ctim)
#endif

void entry_list_init(struct entry_list *list)
{
    list->items = NULL;
    list->len = 0;
    list->cap = 0;
}

void entry_list_free(struct entry_list *list)
{
    size_t i;

    for (i = 0; i < list->len; i++) {
        free(list->items[i].name);
        free(list->items[i].path);
    }
    free(list->items);
    entry_list_init(list);
}

/* Mở rộng mảng (nhân đôi dung lượng) để chứa thêm một entry ở cuối `list`. */
static struct entry *entry_list_reserve(struct entry_list *list)
{
    if (list->len == list->cap) {
        list->cap = list->cap ? list->cap * 2 : 16;
        list->items = xrealloc(list->items, list->cap * sizeof *list->items);
    }
    return &list->items[list->len++];
}

int entry_list_add(struct entry_list *list, const char *name,
                   const char *path, int follow)
{
    struct stat st;
    struct entry *e;

    if (follow) {
        /* Symlink hỏng không đi theo được, nên hiển thị chính symlink đó. */
        if (stat(path, &st) != 0 && lstat(path, &st) != 0)
            return -1;
    } else if (lstat(path, &st) != 0) {
        return -1;
    }

    e = entry_list_reserve(list);
    e->name = xstrdup(name);
    e->path = xstrdup(path);
    e->st = st;
    return 0;
}

void entry_list_move_last(struct entry_list *src, struct entry_list *dst)
{
    *entry_list_reserve(dst) = src->items[--src->len];
}

struct timespec entry_time(const struct stat *st, time_select which)
{
    switch (which) {
    case TIME_CTIME: return ST_CTIM(st);
    case TIME_ATIME: return ST_ATIM(st);
    default:         return ST_MTIM(st);
    }
}
