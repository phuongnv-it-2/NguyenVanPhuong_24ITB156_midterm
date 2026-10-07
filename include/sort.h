/*
 * sort.h - sắp thứ tự danh sách entry (-S, -t, -r, -f).
 */
#ifndef SORT_H
#define SORT_H

#include "entry.h"
#include "options.h"

/* Sắp xếp `list` tại chỗ theo option (không làm gì với -f). */
void sort_entries(struct entry_list *list, const struct ls_options *opt);

#endif /* SORT_H */
