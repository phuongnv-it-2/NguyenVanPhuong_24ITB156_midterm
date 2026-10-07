/*
 * format.h - in các entry (dạng ngắn, -i, -s, -l).
 */
#ifndef FORMAT_H
#define FORMAT_H

#include "entry.h"
#include "options.h"

/* In mọi entry của `list`, mỗi entry một dòng, các cột thẳng hàng.
 * Trả về 0, hoặc 1 nếu có entry không in được đầy đủ. */
int print_entries(const struct entry_list *list, const struct ls_options *opt);

/* In dòng "total N" cho các entry của `list`. */
void print_total(const struct entry_list *list, const struct ls_options *opt);

#endif /* FORMAT_H */
