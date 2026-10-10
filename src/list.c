/*
 * list.c - quyết định liệt kê gì: operand trên dòng lệnh, nội dung
 * thư mục, lọc file ẩn và đệ quy -R.
 */
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "entry.h"
#include "format.h"
#include "list.h"
#include "sort.h"
#include "util.h"

static int is_dot_or_dotdot(const char *name)
{
    return strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
}

/* Entry tên `name` có được hiển thị không (theo -a / -A)? */
static int is_visible(const char *name, const struct ls_options *opt)
{
    if (name[0] != '.' || opt->all)
        return 1;
    if (opt->almost_all)
        return !is_dot_or_dotdot(name);
    return 0;
}

/*
 * Liệt kê nội dung thư mục `path` (và với -R là toàn bộ thư mục con).
 * Trả về 0, hoặc 1 nếu đã báo lỗi nào đó.
 */
static int list_directory(const char *path, const struct ls_options *opt)
{
    struct entry_list list;
    struct dirent *de;
    DIR *dir;
    int status = 0;
    size_t i;

    dir = opendir(path);
    if (dir == NULL)
    {
        warn_errno(path);
        return 1;
    }

    entry_list_init(&list);
    errno = 0;
    while ((de = readdir(dir)) != NULL)
    {
        char *full;

        if (is_visible(de->d_name, opt))
        {
            full = path_join(path, de->d_name);
            /* dùng lstat: symlink hiện như symlink, không đi theo (tránh lặp vô hạn với -R) */
            if (entry_list_add(&list, de->d_name, full, 0) != 0)
            {
                warn_errno(full);
                status = 1;
            }
            free(full);
        }
        errno = 0;
    }
    if (errno != 0)
    {
        warn_errno(path);
        status = 1;
    }
    closedir(dir);

    sort_entries(&list, opt);

    /* Theo man page: "total" luôn in với -l, còn với -s chỉ in khi xuất ra terminal. */
    if (opt->long_format || (opt->show_blocks && isatty(STDOUT_FILENO)))
        print_total(&list, opt);
    if (print_entries(&list, opt) != 0)
        status = 1;

    if (opt->recursive)
    {
        for (i = 0; i < list.len; i++)
        {
            const struct entry *e = &list.items[i];

            if (S_ISDIR(e->st.st_mode) && !is_dot_or_dotdot(e->name))
            {
                printf("\n%s:\n", e->path);
                if (list_directory(e->path, opt) != 0)
                    status = 1;
            }
        }
    }

    entry_list_free(&list);
    return status;
}

int list_operands(int argc, char **argv, const struct ls_options *opt)
{
    static char *default_operand[] = {".", NULL};
    struct entry_list files, dirs;
    int status = 0;
    int printed_something = 0;
    int show_headers;
    int follow;
    size_t i;
    int k;

    if (argc == 0)
    { /* không có operand thì dùng thư mục hiện tại */
        argc = 1;
        argv = default_operand;
    }

    /*
     * Symlink ở operand được đi theo, trừ khi có -d (man page nói rõ)
     * hoặc -l/-n (theo POSIX: hiện chính symlink; man page không nói rõ).
     */
    follow = !(opt->dir_as_file || opt->long_format);

    entry_list_init(&files);
    entry_list_init(&dirs);

    for (k = 0; k < argc; k++)
    {
        if (entry_list_add(&files, argv[k], argv[k], follow) != 0)
        {
            warn_errno(argv[k]);
            status = 1;
            continue;
        }
        /* Thư mục được liệt kê theo nội dung, trừ khi có -d. */
        if (!opt->dir_as_file &&
            S_ISDIR(files.items[files.len - 1].st.st_mode))
            entry_list_move_last(&files, &dirs);
    }

    /* Operand không phải thư mục được in trước, sau đó mới tới từng thư mục.
     * Hai nhóm được sắp xếp riêng. */
    sort_entries(&files, opt);
    sort_entries(&dirs, opt);

    if (files.len > 0)
    {
        if (print_entries(&files, opt) != 0)
            status = 1;
        printed_something = 1;
    }

    /* Tên thư mục chỉ hiện khi liệt kê nhiều operand hoặc dùng -R. */
    show_headers = (argc > 1) || opt->recursive;
    for (i = 0; i < dirs.len; i++)
    {
        if (printed_something)
            putchar('\n');
        if (show_headers)
            printf("%s:\n", dirs.items[i].name);
        if (list_directory(dirs.items[i].path, opt) != 0)
            status = 1;
        printed_something = 1;
    }

    entry_list_free(&files);
    entry_list_free(&dirs);
    return status;
}
