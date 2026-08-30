#!/bin/sh
# 全サンプルを（DS 用ではない）普通の gcc でビルドする。
# Dev Container の中でも、ホストの PC でも動く。
#   ./build.sh          全部ビルドする
#   ./build.sh 04       04_ で始まるものをビルドして実行する
#
# 出力は bin/ に入る（.gitignore 済み）。
set -e

cd "$(dirname "$0")"

if ! command -v gcc >/dev/null 2>&1; then
    echo "gcc が見つかりません。" >&2
    echo >&2
    echo "  Dev Container の中なら: Dockerfile の apt 行に gcc libc6-dev があるか確認し、" >&2
    echo "                          F1 -> Dev Containers: Rebuild Container" >&2
    echo "  ホストで動かすなら:     Ubuntu/WSL -> sudo apt install build-essential" >&2
    echo "                          macOS      -> xcode-select --install" >&2
    exit 1
fi

mkdir -p bin

CFLAGS="-std=c11 -Wall -Wextra -g"
PREFIX=$1          # 空なら全部が対象になる

found=0
for src in ${PREFIX}*.c; do
    [ -e "$src" ] || continue
    found=1
    printf '  CC  %s\n' "$src"
    gcc $CFLAGS -o "bin/$(basename "$src" .c)" "$src"
done

if [ "$found" = 0 ]; then
    echo "'${PREFIX}*.c' にマッチするファイルがありません。" >&2
    exit 1
fi

# 引数で絞り込んだときは、そのまま実行する
[ -n "$PREFIX" ] || exit 0

for src in ${PREFIX}*.c; do
    out="bin/$(basename "$src" .c)"
    printf '\n===== %s =====\n' "$out"
    "./$out"
done
