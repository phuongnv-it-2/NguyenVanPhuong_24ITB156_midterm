/*
 * entry.h - một entry (tên + đường dẫn + dữ liệu stat) và danh sách tự mở rộng.
 */
#ifndef ENTRY_H
#define ENTRY_H

#include <stddef.h>
#include <sys/stat.h>
#include <time.h>

#include "options.h"

struct entry {
    char *name;       /* chuỗi hiển thị cho người dùng */
    char *path;       /* đường dẫn dùng cho stat/readlink/opendir */
    struct stat st;
};

struct entry_list {
    struct entry *items;
    size_t len;
    size_t cap;
};

void entry_list_init(struct entry_list *list);
void entry_list_free(struct entry_list *list);

/*
 * Gọi stat cho `path` rồi thêm entry. Nếu `follow` khác 0 thì đi theo
 * symlink (symlink hỏng thì dùng lstat). Trả về 0 nếu thành công, hoặc
 * -1 kèm errno (khi đó không thêm gì).
 */
int entry_list_add(struct entry_list *list, const char *name,
                   const char *path, int follow);

/* Chuyển entry cuối của `src` sang cuối `dst` (không sao chép chuỗi). */
void entry_list_move_last(struct entry_list *src, struct entry_list *dst);

/* Mốc thời gian do -c / -u chọn (mặc định: thời gian sửa đổi). */
struct timespec entry_time(const struct stat *st, time_select which);

#endif /* ENTRY_H */
