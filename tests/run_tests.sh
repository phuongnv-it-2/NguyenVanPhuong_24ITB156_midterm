#!/bin/sh
# So sánh myls với ls của hệ thống trên một thư mục thử nhỏ.
# Cách dùng (tại thư mục gốc của project):  make && sh tests/run_tests.sh
MYLS="$(pwd)/myls"
T=$(mktemp -d) || exit 1
O=$(mktemp -d) || exit 1   # đầu ra tạm, để ngoài thư mục thử
trap 'rm -rf "$T" "$O"' EXIT
pass=0; fail=0

# Thư mục thử: file thường, file ẩn, symlink, FIFO, thư mục con.
cd "$T" || exit 1
mkdir -p sub/deep empty
echo hello > apple.txt
echo "a bit longer content" > zebra.txt
: > .hidden
printf 'x' > sub/inner.txt
chmod +x zebra.txt
ln -s apple.txt link
ln -s sub dirlink
ln -s nowhere broken
mkfifo fifo
touch -d '2020-01-02 03:04:05' apple.txt
touch -d '2021-05-06 07:08:09' zebra.txt

# Với root, -A luôn bật (theo man page) nên ls tham chiếu cũng cần -A.
REF=""; [ "$(id -u)" -eq 0 ] && REF="-A"

check() { # check "mô tả" "tham số myls" "tham số ls"
    desc=$1; mine=$2; ref=$3

    "$MYLS" $mine >"$O/mine" 2>"$O/err"; rc1=$?

    ls $ref   >"$O/ref" 2>/dev/null
    if cmp -s "$O/mine" "$O/ref"; then pass=$((pass+1))
    else fail=$((fail+1)); echo "FAIL: $desc  (myls $mine)"; diff "$O/mine" "$O/ref" | head -5; fi
}

check "default"              ""          "$REF -1"
check "-a"                   "-a"        "-a -1"
check "-A"                   "-A"        "-A -1"
check "-r"                   "-r"        "$REF -1r"
check "-t"                   "-t"        "$REF -1t"
check "-tr"                  "-tr"       "$REF -1tr"
check "-F"                   "-F"        "$REF -1F"
check "-i"                   "-i"        "$REF -1i"
check "-d on dir"            "-d sub"    "-d sub"
check "-R"                   "-R"        "$REF -1R"
check "multiple operands"    "apple.txt sub empty"  "-1 apple.txt sub empty"
check "-l files"             "-l apple.txt zebra.txt link"  "-l apple.txt zebra.txt link"
check "-n files"             "-n apple.txt zebra.txt"       "-n apple.txt zebra.txt"
check "-lS"                  "-lS apple.txt zebra.txt"      "-lS apple.txt zebra.txt"
check "-lc"                  "-lc apple.txt"                "-lc apple.txt"
check "-lu"                  "-lu apple.txt"                "-lu apple.txt"
check "-lF special"          "-lF fifo zebra.txt"           "-lF fifo zebra.txt"

# Các điểm khác ls của GNU một cách có chủ đích được kiểm tra riêng:
out=$("$MYLS" -lF link); case $out in *"link@ -> apple.txt") pass=$((pass+1));; *) fail=$((fail+1)); echo "FAIL: -F on symlink: $out";; esac
out=$("$MYLS" -l link); case $out in *"link -> apple.txt") pass=$((pass+1));; *) fail=$((fail+1)); echo "FAIL: symlink arrow";; esac
out=$("$MYLS" -l dirlink); case $out in *"dirlink -> sub") pass=$((pass+1));; *) fail=$((fail+1)); echo "FAIL: -l on dir symlink shows link";; esac
out=$("$MYLS" dirlink); [ "$out" = "inner.txt" ] || [ "$out" = "deep
inner.txt" ] && pass=$((pass+1)) || { fail=$((fail+1)); echo "FAIL: dir symlink followed"; }
out=$("$MYLS" -ld zebra.txt); case $out in -rwx*) pass=$((pass+1));; *) fail=$((fail+1)); echo "FAIL: mode string";; esac
out=$("$MYLS" -l apple.txt); case $out in *"Jan  2  2020 apple.txt") pass=$((pass+1));; *) fail=$((fail+1)); echo "FAIL: old date shows year: $out";; esac
out=$("$MYLS" -lh apple.txt); case $out in *" 6B "*) pass=$((pass+1));; *) fail=$((fail+1)); echo "FAIL: -lh: $out";; esac
out=$("$MYLS" -lk sub | head -1); case $out in "total "*) pass=$((pass+1));; *) fail=$((fail+1)); echo "FAIL: -l total line";; esac
out=$("$MYLS" -R | grep -c ':$'); [ "$out" -ge 3 ] && pass=$((pass+1)) || { fail=$((fail+1)); echo "FAIL: -R headers"; }

# Quy tắc ghi đè: option đứng sau thắng
a=$("$MYLS" -ln apple.txt); b=$("$MYLS" -n apple.txt)
[ "$a" = "$b" ] && pass=$((pass+1)) || { fail=$((fail+1)); echo "FAIL: -ln == -n"; }
a=$("$MYLS" -nl apple.txt); b=$("$MYLS" -l apple.txt)
[ "$a" = "$b" ] && pass=$((pass+1)) || { fail=$((fail+1)); echo "FAIL: -nl == -l"; }
a=$("$MYLS" -Rd sub); b=$("$MYLS" -d sub)
[ "$a" = "$b" ] && pass=$((pass+1)) || { fail=$((fail+1)); echo "FAIL: -Rd == -d"; }
a=$("$MYLS" -dR sub | head -1); [ "$a" = "sub:" ] && pass=$((pass+1)) || { fail=$((fail+1)); echo "FAIL: -dR recurses"; }

# Lỗi và exit status
"$MYLS" nonexistent 2>/dev/null; [ $? -ne 0 ] && pass=$((pass+1)) || { fail=$((fail+1)); echo "FAIL: exit status for missing file"; }
"$MYLS" -Z 2>/dev/null;          [ $? -ne 0 ] && pass=$((pass+1)) || { fail=$((fail+1)); echo "FAIL: exit status for bad option"; }
"$MYLS" apple.txt >/dev/null;    [ $? -eq 0 ] && pass=$((pass+1)) || { fail=$((fail+1)); echo "FAIL: exit status 0"; }
out=$("$MYLS" apple.txt nonexistent 2>/dev/null); [ "$out" = "apple.txt" ] && pass=$((pass+1)) || { fail=$((fail+1)); echo "FAIL: continues after error"; }

# --- Các ca biên bổ sung ---------------------------------------------------
# -d không có operand: liệt kê chính "."
out=$("$MYLS" -d); [ "$out" = "." ] && pass=$((pass+1)) || { fail=$((fail+1)); echo "FAIL: -d without operand"; }

# -f: không sắp xếp (cùng thứ tự readdir với ls -f, bỏ . và ..)
a=$("$MYLS" -f sub); b=$(ls -f sub | grep -v '^\.\.\?$')
[ "$a" = "$b" ] && pass=$((pass+1)) || { fail=$((fail+1)); echo "FAIL: -f order"; }

# -S và -t ghi đè nhau: option đứng sau thắng
a=$("$MYLS" -St . | head -1); b=$("$MYLS" -t . | head -1)
[ "$a" = "$b" ] && pass=$((pass+1)) || { fail=$((fail+1)); echo "FAIL: -St == -t"; }
a=$("$MYLS" -tS .); b=$("$MYLS" -S .)
[ "$a" = "$b" ] && pass=$((pass+1)) || { fail=$((fail+1)); echo "FAIL: -tS == -S"; }

# Vòng lặp symlink: không treo, không crash, hiển thị như symlink
ln -s loop2 loop1; ln -s loop1 loop2
out=$("$MYLS" loop1); [ "$out" = "loop1" ] && pass=$((pass+1)) || { fail=$((fail+1)); echo "FAIL: symlink loop"; }
"$MYLS" -R . >/dev/null 2>&1; [ $? -eq 0 ] && pass=$((pass+1)) || { fail=$((fail+1)); echo "FAIL: -R with symlink loop"; }

# Operand rỗng là lỗi, không phải "."
"$MYLS" "" >/dev/null 2>&1; [ $? -ne 0 ] && pass=$((pass+1)) || { fail=$((fail+1)); echo "FAIL: empty operand"; }

# Thông báo lỗi option bắt đầu bằng tên chương trình
out=$("$MYLS" -Z 2>&1 | head -1); case $out in "myls: invalid option"*) pass=$((pass+1));; *) fail=$((fail+1)); echo "FAIL: option error text: $out";; esac

# Lỗi ghi stdout (đầy đĩa) phải được báo và trả về exit status khác 0
if [ -w /dev/full ]; then
    "$MYLS" . >/dev/full 2>/dev/null; [ $? -ne 0 ] && pass=$((pass+1)) || { fail=$((fail+1)); echo "FAIL: write error"; }
fi

# uid/gid không có trong hệ thống phải hiển thị bằng số (cần quyền root để chown)
if [ "$(id -u)" -eq 0 ]; then
    : > ghost; chown 54321:54321 ghost
    out=$("$MYLS" -l ghost | awk '{print $3, $4}'); [ "$out" = "54321 54321" ] && pass=$((pass+1)) || { fail=$((fail+1)); echo "FAIL: numeric uid/gid fallback: $out"; }
fi

# -q / -w với tên file chứa ký tự điều khiển
touch "$(printf 'bad\001name')"
out=$("$MYLS" -q | grep 'bad'); [ "$out" = "bad?name" ] && pass=$((pass+1)) || { fail=$((fail+1)); echo "FAIL: -q"; }
out=$("$MYLS" -w | grep -c "$(printf 'bad\001name')"); [ "$out" -eq 1 ] && pass=$((pass+1)) || { fail=$((fail+1)); echo "FAIL: -w"; }

# Thư mục không có quyền đọc (bỏ qua với root vì root đọc được hết)
if [ "$(id -u)" -ne 0 ]; then
    mkdir secret; chmod 000 secret
    "$MYLS" secret 2>/dev/null; [ $? -ne 0 ] && pass=$((pass+1)) || { fail=$((fail+1)); echo "FAIL: unreadable dir"; }
    chmod 755 secret
fi

echo "passed: $pass, failed: $fail"
[ "$fail" -eq 0 ]
