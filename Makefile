# Makefile của myls - bản ls(1) rút gọn.
#
# Viết theo cú pháp chung nên chạy được với CẢ GNU make (Linux) lẫn BSD make
# (lệnh `make` mặc định của NetBSD): không dùng wildcard, patsubst hay
# pattern rule (%), chỉ dùng suffix rule .c.o và danh sách file tường minh.

PROG     = myls
OBJS     = src/entry.o src/format.o src/list.o src/main.o \
           src/options.o src/sort.o src/util.o
HEADERS  = include/entry.h include/format.h include/list.h \
           include/options.h include/sort.h include/util.h

# Dùng gnu11 (không dùng c11 "nghiêm ngặt") để thư viện C của Linux lẫn NetBSD
# đều khai báo đủ các hàm POSIX/BSD cần thiết (lstat, major/minor, st_mtim...).
SANITIZE =
CFLAGS   = -g -O2 -std=gnu11 -Wall -Wextra -Wpedantic -Iinclude \
           -D_FILE_OFFSET_BITS=64 $(SANITIZE)

.SUFFIXES: .c .o
.PHONY: all clean test asan

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

# Sửa file .h thì biên dịch lại tất cả file .o.
$(OBJS): $(HEADERS)

test: $(PROG)
	sh tests/run_tests.sh

# Build với AddressSanitizer + UBSan (cần trình biên dịch hỗ trợ; trên NetBSD
# có thể không có). Chạy `make clean` sau khi dùng xong.
asan:
	$(MAKE) clean
	$(MAKE) SANITIZE="-fsanitize=address,undefined -fno-omit-frame-pointer"

clean:
	rm -f $(OBJS) $(PROG)
	rm -rf build
