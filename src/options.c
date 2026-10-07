/*
 * options.c - phân tích dòng lệnh bằng getopt(3).
 *
 * Các option "ghi đè nhau" (-h/-k, -l/-n, -c/-u, -R/-d, -q/-w, -S/-t)
 * được xử lý theo quy tắc: option đứng sau cùng thắng.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "options.h"
#include "util.h"

#define OPTSTRING "AacdFfhiklnqRrSstuw"
#define DEFAULT_BLOCK 512

static void usage(void)
{
    fprintf(stderr, "usage: %s [-%s] [file ...]\n", PROG_NAME, OPTSTRING);
}

/*
 * Đơn vị block lấy từ $BLOCKSIZE: một số, có thể kèm hậu tố k/m/g.
 * Giá trị sai thì dùng 512 byte; giá trị nhỏ hơn 512 được nâng lên 512.
 */
static uintmax_t blocksize_from_env(void)
{
    const char *env = getenv("BLOCKSIZE");
    char *end;
    unsigned long long n;

    if (env == NULL || *env == '\0')
        return DEFAULT_BLOCK;
    n = strtoull(env, &end, 10);
    if (end == env || n == 0)
        return DEFAULT_BLOCK;
    switch (toupper((unsigned char)*end)) {
    case '\0':                                  break;
    case 'K': n *= 1024ULL;                     break;
    case 'M': n *= 1024ULL * 1024;              break;
    case 'G': n *= 1024ULL * 1024 * 1024;       break;
    default:  return DEFAULT_BLOCK;
    }
    return n < DEFAULT_BLOCK ? DEFAULT_BLOCK : (uintmax_t)n;
}

int parse_options(int argc, char **argv, struct ls_options *opt,
                  int *first_operand)
{
    int c;
    int name_given = 0;

    memset(opt, 0, sizeof *opt);
    opt->time_sel = TIME_MTIME;
    opt->sort_by = SORT_NAME;
    opt->block_unit = blocksize_from_env();

    while ((c = getopt(argc, argv, OPTSTRING)) != -1) {
        switch (c) {
        case 'A': opt->almost_all = 1; break;
        case 'a': opt->all = 1; break;
        case 'c': opt->time_sel = TIME_CTIME; break;
        case 'u': opt->time_sel = TIME_ATIME; break;
        case 'd': opt->dir_as_file = 1; opt->recursive = 0; break;
        case 'R': opt->recursive = 1; opt->dir_as_file = 0; break;
        case 'F': opt->classify = 1; break;
        case 'f': opt->unsorted = 1; break;
        case 'h': opt->human = 1; opt->kilo = 0; break;
        case 'k': opt->kilo = 1; opt->human = 0; break;
        case 'i': opt->show_inode = 1; break;
        case 'l': opt->long_format = 1; opt->numeric_ids = 0; break;
        case 'n': opt->long_format = 1; opt->numeric_ids = 1; break;
        case 'q': opt->name_mode = NAME_QUESTION; name_given = 1; break;
        case 'w': opt->name_mode = NAME_RAW; name_given = 1; break;
        case 'r': opt->reverse = 1; break;
        case 'S': opt->sort_by = SORT_SIZE; break;
        case 't': opt->sort_by = SORT_TIME; break;
        case 's': opt->show_blocks = 1; break;
        default:  /* getopt đã tự in thông báo lỗi */
            usage();
            return -1;
        }
    }

    /* Man page: -A luôn được bật với super-user. */
    if (geteuid() == 0)
        opt->almost_all = 1;

    /* Mặc định của -q/-w phụ thuộc đầu ra có phải terminal hay không. */
    if (!name_given)
        opt->name_mode = isatty(STDOUT_FILENO) ? NAME_QUESTION : NAME_RAW;

    *first_operand = optind;
    return 0;
}
