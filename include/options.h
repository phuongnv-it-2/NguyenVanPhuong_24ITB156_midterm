/*
 * options.h - xử lý option dòng lệnh của myls.
 */
#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdint.h>

/* Mốc thời gian dùng cho sắp xếp -t và in -l (-c / -u). */
typedef enum { TIME_MTIME, TIME_CTIME, TIME_ATIME } time_select;

/* Khóa sắp xếp (-S / -t); mặc định là theo tên. */
typedef enum { SORT_NAME, SORT_SIZE, SORT_TIME } sort_key;

/* Cách hiển thị ký tự không in được trong tên (-q / -w). */
typedef enum { NAME_RAW, NAME_QUESTION } name_style;

/* Mọi thiết lập có thể thay đổi từ dòng lệnh. */
struct ls_options {
    int all;           /* -a: hiện mọi entry, kể cả "." và ".." */
    int almost_all;    /* -A: hiện file ẩn, trừ "." và ".." */
    int dir_as_file;   /* -d: liệt kê thư mục như file thường */
    int classify;      /* -F: thêm / * @ % = | sau tên */
    int human;         /* -h: kích thước dễ đọc */
    int show_inode;    /* -i: in số inode */
    int kilo;          /* -k: đếm block theo đơn vị 1024 byte */
    int long_format;   /* -l hoặc -n */
    int numeric_ids;   /* -n: hiện uid/gid dạng số trong định dạng dài */
    int recursive;     /* -R: đệ quy vào thư mục con */
    int reverse;       /* -r: đảo thứ tự sắp xếp */
    int show_blocks;   /* -s: in số block */
    int unsorted;      /* -f: không sắp xếp */
    time_select time_sel;
    sort_key sort_by;
    name_style name_mode;
    uintmax_t block_unit; /* đơn vị của -s (BLOCKSIZE, mặc định 512) */
};

/*
 * Phân tích argv. Thành công trả về 0 và lưu chỉ số operand đầu tiên vào
 * *first_operand. Nếu lỗi thì in usage và trả về -1.
 */
int parse_options(int argc, char **argv, struct ls_options *opt,
                  int *first_operand);

#endif /* OPTIONS_H */
