/*
 * main.c - điểm bắt đầu của myls, bản ls(1) rút gọn.
 */
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#include "list.h"
#include "options.h"
#include "util.h"

int main(int argc, char **argv)
{
    struct ls_options opt;
    int first_operand;
    int status;

    setlocale(LC_ALL, ""); /* cần cho tên nhiều byte (-q) và định dạng ngày */

    if (parse_options(argc, argv, &opt, &first_operand) != 0)
        return EXIT_FAILURE;

    status = list_operands(argc - first_operand, argv + first_operand, &opt);

    /* Báo lỗi ghi (ví dụ đầy đĩa) thay vì bỏ qua. */
    if (fflush(stdout) != 0 || ferror(stdout)) {
        warn_errno("stdout");
        status = 1;
    }
    return status ? EXIT_FAILURE : EXIT_SUCCESS;
}
