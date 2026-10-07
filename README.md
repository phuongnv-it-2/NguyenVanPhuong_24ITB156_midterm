# Midterm Project – Implement `ls(1)`

## 1. Thông tin sinh viên

|                       |                                                                   |
| --------------------- | ----------------------------------------------------------------- |
| **Họ tên**            | Nguyễn Văn Phương                                                 |
| **Mã sinh viên**      | 24ITB156                                                          |
| **GitHub repository** | https://github.com/phuongnv-it-2/NguyenVanPhuong_24ITB156_midterm |

## 2. Giới thiệu

`myls` là phiên bản rút gọn của lệnh UNIX `ls(1)`, viết bằng C từ đầu, bám theo
manual page NetBSD `ls(1)` được cung cấp trong đề bài. Chương trình chỉ dùng
các lời gọi hệ thống/thư viện chuẩn POSIX: `opendir/readdir/closedir`,
`stat/lstat`, `readlink`, `getpwuid/getgrgid`, `getopt`, `qsort`, `strftime`,
`isatty`.

```
myls [-AacdFfhiklnqRrSstuw] [file ...]
```

## 3. Các chức năng đã cài đặt

| Option | Chức năng                                                                                          |
| ------ | -------------------------------------------------------------------------------------------------- |
| `-A`   | Hiện mọi entry trừ `.` và `..`. **Luôn bật khi chạy bằng root** (đúng như manual).                 |
| `-a`   | Hiện tất cả entry kể cả `.` và `..`.                                                               |
| `-c`   | Dùng thời gian đổi trạng thái (ctime) cho `-t` và `-l`.                                            |
| `-d`   | Liệt kê thư mục như file thường (không đi vào trong, không đi theo symlink ở operand).             |
| `-F`   | Thêm ký hiệu sau tên: `/` thư mục, `*` thực thi, `@` symlink, `%` whiteout, `=` socket, `\|` FIFO. |
| `-f`   | Không sắp xếp.                                                                                     |
| `-h`   | Kích thước dạng dễ đọc (`1.5K`, `23M`…) cho `-l` và `-s`; ghi đè `-k`.                             |
| `-i`   | In số inode.                                                                                       |
| `-k`   | Với `-s`, block tính theo đơn vị 1024 byte; ghi đè `-h`.                                           |
| `-l`   | Định dạng dài: mode, số link, owner, group, kích thước, ngày giờ, tên (`-> đích` với symlink).     |
| `-n`   | Như `-l` nhưng owner/group hiển thị bằng số.                                                       |
| `-q`   | Ký tự không in được trong tên → `?` (mặc định khi output là terminal).                             |
| `-w`   | In nguyên ký tự thô (mặc định khi output không phải terminal).                                     |
| `-R`   | Liệt kê đệ quy thư mục con.                                                                        |
| `-r`   | Đảo ngược thứ tự sắp xếp.                                                                          |
| `-S`   | Sắp xếp theo kích thước, lớn nhất trước.                                                           |
| `-s`   | In số block thực sự dùng (đơn vị 512 byte hoặc `BLOCKSIZE`, làm tròn lên).                         |
| `-t`   | Sắp xếp theo thời gian, mới nhất trước (bằng nhau thì theo tên).                                   |
| `-u`   | Dùng thời gian truy cập (atime) cho `-t` và `-l`.                                                  |

### Các cặp option ghi đè nhau (option **đứng sau cùng** thắng)

`-h`/`-k`, `-l`/`-n`, `-c`/`-u`, `-R`/`-d`, `-q`/`-w` (và `-S`/`-t` cho khóa sắp xếp).

### Hành vi theo manual

- Không có operand → liệt kê thư mục hiện tại.
- Operand không phải thư mục được in **trước**; nhóm file và nhóm thư mục được sắp xếp **riêng**, theo thứ tự từ điển (`strcmp`).
- Có nhiều operand hoặc dùng `-R` thì mỗi thư mục có dòng tiêu đề `tên:` và cách nhau bằng dòng trống.
- Mặc định mỗi entry một dòng.
- `-i`, `-s`, `-l` được ngăn cách bằng khoảng trắng và các cột được căn thẳng hàng.
- Long format: bit setuid/setgid (`s`/`S`), sticky (`t`/`T`); file thiết bị hiển thị `major, minor` ở cột kích thước; dòng `total N` được in trước nội dung mỗi thư mục (với `-s` thì chỉ in khi output là terminal, như manual).
- Ngày: file trong vòng ~6 tháng hiển thị `Mon dd HH:MM`, file cũ hơn (hoặc ở tương lai) hiển thị `Mon dd  YYYY`. Biến môi trường `TZ` được thư viện C áp dụng.
- `BLOCKSIZE` (số, có thể kèm `k`/`m`/`g`) quyết định đơn vị của `-s` khi không có `-h`/`-k`.
- Exit status `0` khi thành công, `1` khi có lỗi (file không tồn tại, không có quyền, option sai…). Chương trình báo lỗi rồi **tiếp tục** với operand còn lại.

### Quyết định thiết kế / giới hạn

- Symlink ở operand được đi theo (như `stat`), **trừ** khi có `-d` hoặc `-l`/`-n` (theo POSIX: khi đó hiển thị chính symlink). Symlink hỏng luôn được hiển thị như symlink.
- Symlink bên trong thư mục không bao giờ bị đi theo, nên `-R` không bị lặp vô hạn.
- `-f` chỉ tắt sắp xếp (manual không nói nó bật `-a`).
- `-F` đặt ký hiệu ngay sau tên, kể cả với symlink (`link@ -> đích`).
- Trạng thái archive `a`/`A` cần cờ file của BSD (`st_flags`), không có trên Linux nên không hiển thị; `w` (whiteout) chỉ có trên hệ thống định nghĩa `S_IFWHT`.
- Hết bộ nhớ → in thông báo `out of memory` và thoát (không segfault).

## 4. Cấu trúc project

```
NguyenVanPhuong_24ITB156_midterm/
├── .gitignore
├── Makefile
├── README.md
├── include/
│   ├── entry.h      # struct entry, danh sách entry
│   ├── format.h     # in kết quả
│   ├── list.h       # xử lý operand, đọc thư mục, đệ quy
│   ├── options.h    # cấu trúc option và parse_options
│   ├── sort.h       # sắp xếp
│   └── util.h       # tiện ích chung
├── src/
│   ├── entry.c      # lstat/stat, danh sách động, chọn ctime/atime/mtime
│   ├── format.c     # -i, -s, -l, -F, căn cột, ngày giờ, owner/group
│   ├── list.c       # operand, readdir, lọc file ẩn, -R
│   ├── main.c
│   ├── options.c    # getopt, quy tắc "option sau thắng", BLOCKSIZE
│   ├── sort.c       # tên / -S / -t / -r / -f
│   └── util.c       # xmalloc, mode string, humanize, in tên (-q/-w)
└── tests/
    └── run_tests.sh # so sánh với ls hệ thống
```

## 5. Hướng dẫn biên dịch và chạy

### 5.1. Yêu cầu

Linux hoặc UNIX (Ubuntu, Debian, macOS, WSL…) có `gcc` (hoặc `cc`), `make` và `git`.

```bash
# Ubuntu / Debian / WSL: cài công cụ nếu chưa có
sudo apt update && sudo apt install -y build-essential git
```

### 5.2. Lấy mã nguồn và biên dịch

```bash
git clone https://github.com/phuongnv-it-2/NguyenVanPhuong_24ITB156_midterm.git
cd NguyenVanPhuong_24ITB156_midterm
make            # biên dịch, tạo file thực thi ./myls
```

Các lệnh khác của Makefile:

```bash
make test       # chạy bộ test (so sánh với ls của hệ thống)
make clean
```

### 5.3. Cú pháp

```
./myls [-AacdFfhiklnqRrSstuw] [file ...]
```

Các option có thể viết gộp (`-lah`) hoặc tách rời (`-l -a -h`). Dùng `--` để
kết thúc option, ví dụ `./myls -- -tên-file-bắt-đầu-bằng-dấu-gạch`. Không có
operand thì chương trình liệt kê thư mục hiện tại.

### 5.4. Ví dụ

```bash
./myls                      # thư mục hiện tại, mỗi file một dòng
./myls -a                   # thêm file ẩn, kể cả . và ..
./myls -l /etc              # định dạng dài
./myls -lh src              # định dạng dài, kích thước dễ đọc (1.5K, 23M)
./myls -lS include          # sắp xếp theo kích thước, lớn nhất trước
./myls -ltr                 # sắp xếp theo thời gian, cũ nhất trước
./myls -lu README.md        # hiện thời gian truy cập (atime)
./myls -R src               # liệt kê đệ quy
./myls -liF README.md src   # inode + ký hiệu loại file, nhiều operand
./myls -ld /tmp             # thư mục /tmp như một file (thấy bit sticky 't')
./myls -n /etc/passwd       # owner/group hiển thị bằng số
./myls -s src               # số block (đơn vị 512 byte)
BLOCKSIZE=1k ./myls -s src  # đổi đơn vị block bằng biến môi trường
./myls -q                   # tên có ký tự lạ hiện '?'
./myls /khong/ton/tai       # báo lỗi, exit status 1
```

Kiểm tra exit status của lần chạy vừa rồi: `echo $?` (0 là thành công, 1 là có lỗi).

### 5.5. So sánh với `ls` hệ thống

```bash
diff <(./myls -a src) <(ls -1a src) && echo "giống nhau"
```

Một vài khác biệt có chủ ý so với `ls` của GNU được liệt kê ở mục 3
(đơn vị block 512 byte, `-F` đặt ký hiệu sau tên symlink, `-f` không bật `-a`).

### 5.6. Xử lý sự cố

| Triệu chứng                                 | Cách xử lý                                                      |
| ------------------------------------------- | --------------------------------------------------------------- |
| `make: command not found` / `cc: not found` | Cài `build-essential` (xem 5.1).                                |
| `Permission denied` khi chạy `./myls`       | `chmod +x myls` hoặc chạy lại `make`.                           |
| Kết quả `-A` luôn có file ẩn                | Đang chạy bằng root; theo manual, `-A` luôn bật với super-user. |
| Thay đổi mã mà chạy không đổi               | `make clean && make`.                                           |

## 6. Kiểm thử

- `tests/run_tests.sh` tạo thư mục thử (file thường, file ẩn, symlink thường và hỏng, FIFO, thư mục con) và so sánh output với `ls` hệ thống cho `-a -A -r -t -tr -F -i -d -R -l -n -lS -lc -lu`, cùng các kiểm tra riêng về: mũi tên symlink, định dạng ngày, `-h`, quy tắc ghi đè option, exit status, `-q`/`-w`, tiếp tục sau lỗi.
- Đã chạy thêm với AddressSanitizer + UBSan trên nhiều tổ hợp option, thư mục 3000 file, `/dev`, symlink dài, thư mục rỗng và thư mục không có quyền đọc — không có lỗi bộ nhớ.
- Makefile bật `-Wall -Wextra -Wpedantic`, biên dịch không có warning.

## 7. Git

```bash
git init
git add .
git commit -m "Implement ls: options, sorting, long format, recursion"
git branch -M main
git remote add origin https://github.com/phuongnv-it-2/NguyenVanPhuong_24ITB156_midterm.git
git push -u origin main
```

`.gitignore` loại trừ `myls`, thư mục `build/`, `*.o`, `*.d`, file core và file tạm của editor.
