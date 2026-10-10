#!/bin/sh
# So sanh tung truong hop cua ./myls voi ls he thong (chay tren NetBSD).
# Cach dung:  sh compare_ls.sh        (trong thu muc co ./myls)
# Moi truong hop in GIONG hoac KHAC; neu KHAC thi in diff (toi da 15 dong).
# stderr duoc tach rieng, ten chuong trinh (ls:/myls:) duoc chuan hoa.

MYLS=./myls
T=/tmp/cmp_ls_$$
A=$T.a; B=$T.b
pass=0; fail=0

# Thu muc thu co dinh de ket qua on dinh, khong phu thuoc /etc
D=$T.dir
rm -rf "$D"; mkdir -p "$D/sub/deep" "$D/empty"
: > "$D/apple.txt"; : > "$D/zebra.txt"; : > "$D/.hidden"; : > "$D/sub/x"
echo hello > "$D/b.txt"
head -c 1000 /dev/zero > "$D/f1000"
head -c 1023 /dev/zero > "$D/f1023"
head -c 1024 /dev/zero > "$D/f1024"
head -c 1536 /dev/zero > "$D/f1536"
head -c 10240 /dev/zero > "$D/f10k"
head -c 1048576 /dev/zero > "$D/f1m"
chmod 755 "$D/apple.txt"; chmod 4755 "$D/zebra.txt"
ln -s apple.txt "$D/link"; ln -s sub "$D/dlink"; ln -s nowhere "$D/broken"
mkfifo "$D/ff"

# t "mo ta" lenh... : chay ca hai chuong trinh voi cung doi so (cd vao $D)
t() {
    desc=$1; shift
    ( cd "$D" && ls "$@" 2>"$T.e1" | cat >"$A" )
    ( cd "$D" && "$OLDPWD/$MYLS" "$@" 2>"$T.e2" | cat >"$B" )
    sed 's/^ls: /X: /' "$T.e1" > "$T.e1n"
    sed 's/^myls: /X: /' "$T.e2" > "$T.e2n"
    if cmp -s "$A" "$B"; then
        r="GIONG"; pass=$((pass+1))
    else
        r="KHAC "; fail=$((fail+1))
    fi
    se=""
    cmp -s "$T.e1n" "$T.e2n" || se="  [stderr khac]"
    echo "$r  ls $desc$se"
    if [ "$r" = "KHAC " ]; then diff "$A" "$B" | head -15; fi
    if [ -n "$se" ]; then diff "$T.e1n" "$T.e2n" | head -6; fi
}

# --- tung option rieng le ---
t "(khong option)"
t "-a" -a
t "-A" -A
t "-c" -c
t "-d" -d
t "-d sub link dlink" -d sub link dlink
t "-F" -F
t "-f" -f
t "-i" -i
t "-k -s" -k -s
t "-s" -s
t "-q" -q
t "-r" -r
t "-S" -S
t "-t" -t
t "-u" -u
t "-w" -w
t "-R" -R
# --- long format ---
t "-l" -l
t "-n" -n
t "-lh" -lh
t "-lc" -lc
t "-lu" -lu
t "-lt" -lt
t "-lS" -lS
t "-lF" -lF
t "-li" -li
t "-ls" -ls
t "-lsi" -lsi
t "-lR" -lR
t "-lA" -lA
t "-la" -la
# --- ghi de nhau (option sau cung thang) ---
t "-lh -k -s" -h -k -s
t "-k -h -s" -k -h -s
t "-l -n" -l -n
t "-n -l" -n -l
t "-c -u -lt" -c -u -lt
t "-u -c -lt" -u -c -lt
t "-q -w" -q -w
t "-w -q" -w -q
t "-R -d" -R -d
t "-d -R" -d -R
# --- operand ---
t "sub empty" sub empty
t "apple.txt sub" apple.txt sub
t "-l link dlink" -l link dlink
t "-F link dlink" -F link dlink
t "-R sub" -R sub
t "-R sub empty" -R sub empty
t "-l empty" -l empty
t "-lh f1000 f1023 f1024 f1536 f10k f1m" -lh f1000 f1023 f1024 f1536 f10k f1m
t "-s f1000 f1024 f1m" -s f1000 f1024 f1m
t "nonexist" nonexist
t "nonexist sub" nonexist sub
t "-- -l" -- -l
# --- thu muc he thong ---
t "-l /etc" -l /etc
t "-lh /etc" -lh /etc
t "-R /etc (chi stdout)" -R /etc
t "-l /dev/null /dev/tty" -l /dev/null /dev/tty
t "-ld /tmp" -ld /tmp

echo
echo "TONG KET: GIONG=$pass  KHAC=$fail"
rm -rf "$D" "$T".*
