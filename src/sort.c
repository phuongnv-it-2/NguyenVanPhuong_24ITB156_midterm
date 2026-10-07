/*
 * sort.c - sắp xếp các entry.
 *
 * Mặc định sắp theo tên (từ điển). -S sắp theo kích thước (lớn trước),
 * -t theo thời gian (mới trước); khóa bằng nhau thì xét theo tên.
 * -r đảo toàn bộ thứ tự. -f tắt sắp xếp.
 */
#include <stdlib.h>
#include <string.h>

#include "sort.h"

/* qsort(3) không cho truyền thêm dữ liệu vào hàm so sánh nên phải dùng biến static. */
static const struct ls_options *cmp_opt;

static int compare_entries(const void *pa, const void *pb)
{
    const struct entry *a = pa;
    const struct entry *b = pb;
    int result = 0;

    if (cmp_opt->sort_by == SORT_SIZE) {
        if (a->st.st_size != b->st.st_size)
            result = (a->st.st_size > b->st.st_size) ? -1 : 1;
    } else if (cmp_opt->sort_by == SORT_TIME) {
        struct timespec ta = entry_time(&a->st, cmp_opt->time_sel);
        struct timespec tb = entry_time(&b->st, cmp_opt->time_sel);

        if (ta.tv_sec != tb.tv_sec)
            result = (ta.tv_sec > tb.tv_sec) ? -1 : 1;
        else if (ta.tv_nsec != tb.tv_nsec)
            result = (ta.tv_nsec > tb.tv_nsec) ? -1 : 1;
    }
    if (result == 0)
        result = strcmp(a->name, b->name);
    return cmp_opt->reverse ? -result : result;
}

void sort_entries(struct entry_list *list, const struct ls_options *opt)
{
    if (opt->unsorted || list->len < 2)
        return;
    cmp_opt = opt;
    qsort(list->items, list->len, sizeof *list->items, compare_entries);
}
