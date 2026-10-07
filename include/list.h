/*
 * list.h - xử lý operand, đọc thư mục và đệ quy.
 */
#ifndef LIST_H
#define LIST_H

#include "options.h"

/*
 * Liệt kê các operand (argc có thể bằng 0, nghĩa là "."). Trả về 0 nếu
 * thành công, 1 nếu đã báo lỗi nào đó.
 */
int list_operands(int argc, char **argv, const struct ls_options *opt);

#endif /* LIST_H */
