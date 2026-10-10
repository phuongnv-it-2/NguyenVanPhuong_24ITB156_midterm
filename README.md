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
- Chỉ khi có **nhiều operand** thì mỗi thư mục có dòng tiêu đề `tên:`; với `-R`, các thư mục **con** luôn có tiêu đề (thư mục gốc duy nhất thì không), các khối cách nhau bằng dòng trống — đúng như `ls` của NetBSD.
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
- Đường dẫn dài hơn `PATH_MAX` (cây thư mục lồng quá sâu) không làm chương trình crash: `myls` báo `File name too long` cho nhánh đó rồi tiếp tục các nhánh khác (exit status 1).
- Lỗi ghi ra stdout (ví dụ `> /dev/full`) được phát hiện và báo cáo bằng exit status 1.

## 4. Cấu trúc project

```
NguyenVanPhuong_24ITB156_midterm/
├── .gitignore
├── Makefile         # chạy được với cả GNU make và BSD make (NetBSD)
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
make asan       # build với AddressSanitizer + UBSan để bắt lỗi bộ nhớ
make clean      # xóa các file .o và myls (cũng dùng sau make asan)
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

- `make test` hiện chạy **50 test, tất cả đều qua** (đã kiểm tra trên Linux với GNU make/bmake và trên NetBSD 10.1 thật).
- `tests/run_tests.sh` tạo thư mục thử (file thường, file ẩn, symlink thường và hỏng, FIFO, thư mục con) và so sánh output với `ls` hệ thống cho `-a -A -r -t -tr -F -i -d -R -l -n -lS -lc -lu`, cùng các kiểm tra riêng về: mũi tên symlink, định dạng ngày, `-h`, quy tắc ghi đè option (kể cả `-S`/`-t`), exit status, `-q`/`-w`, tiếp tục sau lỗi, `-d` không operand, `-f`, vòng lặp symlink, operand rỗng, lỗi ghi `/dev/full`, uid/gid không tồn tại hiển thị bằng số.
- Đã chạy thêm với AddressSanitizer + UBSan trên nhiều tổ hợp option, thư mục 3000 file, `/dev`, symlink dài, thư mục rỗng và thư mục không có quyền đọc — không có lỗi bộ nhớ. Có thể tự chạy lại bằng `make asan && make test`.
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

`.gitignore` loại trừ file thực thi `myls`, các file object `*.o`, `*.d`, file core và file tạm của editor.

## 8. Chạy trên NetBSD

Chương trình dùng trình biên dịch `cc` (gcc) và BSD `make` có sẵn của NetBSD; Makefile viết theo cú pháp chung nên **không cần cài `gmake`**. Đã kiểm tra build bằng GNU make và BSD make (`bmake`) trên Linux; nên chạy lại `make test` trên máy NetBSD của bạn để xác nhận.

```sh
# 1. Lấy mã nguồn (cách không cần cài thêm gì: dùng ftp có sẵn của NetBSD)
ftp -o myls.tar.gz https://github.com/phuongnv-it-2/NguyenVanPhuong_24ITB156_midterm/archive/refs/heads/main.tar.gz
tar xzf myls.tar.gz
cd NguyenVanPhuong_24ITB156_midterm-main

# 2. Biên dịch và kiểm thử
make
make test

# 3. Chạy
./myls -l /etc
```

Nếu muốn dùng `git`: `export PKG_PATH="https://cdn.NetBSD.org/pub/pkgsrc/packages/NetBSD/$(uname -p)/$(uname -r)/All"` rồi `pkg_add pkgin && pkgin install git` (cần quyền root), sau đó `git clone` như mục 5.2.

So sánh với `ls` của NetBSD:

```sh
./myls -l /etc > /tmp/a.txt; ls -l /etc > /tmp/b.txt; diff /tmp/a.txt /tmp/b.txt && echo GIONG
```

Định dạng cột của `-l` theo NetBSD: hai dấu cách sau mode, owner và group.

## 9. Kết quả đối chiếu với `ls` của NetBSD 10.1

Đã chạy trên máy NetBSD 10.1 thật (người dùng thường, không phải root):

| Kiểm tra | Kết quả |
| -------- | ------- |
| `make` | Biên dịch sạch, không warning |
| `make test` | `passed: 50, failed: 0` |
| `-l`, `-la`, `-lS`, `-lt`, `-F`, `-i`, `-s`, `-lsi` trên `/etc` | Giống `ls` NetBSD từng ký tự |
| `-l /dev/null /dev/tty` (cột major, minor của file thiết bị) | Giống |
| `-lh /etc` | Còn khác (xem bên dưới) |
| `-R /etc` | Còn khác (xem bên dưới) |

### Các điểm còn khác biệt đã biết

- **`-lh`**: cách làm tròn/đổi đơn vị của `humanize_number(3)` trong `ls` NetBSD có thể khác `myls` (`myls` đổi đơn vị từ 1024, hiển thị `1.5K`, `23M`…). Manual của đề chỉ yêu cầu "kích thước theo bytes ở dạng dễ đọc", không quy định chính xác cách làm tròn.
- **`-R /etc`**: phép so sánh ở trên gộp cả stderr; khi chạy bằng người dùng thường, thư mục không có quyền đọc làm thông báo lỗi khác tên chương trình (`ls:` và `myls:`) và có thể xen kẽ khác thứ tự. Phần output chính chưa được xác nhận là khác hay không.

Hai điểm này không ảnh hưởng tới các hành vi bắt buộc trong manual của đề (các option, thứ tự sắp xếp, định dạng long, exit status).

