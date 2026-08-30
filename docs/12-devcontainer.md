# 12. 開発環境（Dev Container）

> **合宿前にこれだけはやっておいてください。**
> 当日「環境構築で半日溶けた」が一番もったいないです。
>
> **ゴール**
>
> - Dev Container が起動する
> - `make` で `.nds` ができる
> - melonDS で起動して「Hello, DS!」が出る

---

## 12.1 なにを使うのか

**ローカルには何もインストールしません。** Docker のコンテナの中に開発環境が全部入っています。

```
あなたの PC                          コンテナの中（Ubuntu）
┌──────────────────┐              ┌────────────────────────────┐
│ VS Code          │─── 接続 ────▶│ arm-none-eabi-gcc          │
│ melonDS          │              │ libnds / maxmod            │
│ プロジェクトの   │◀── 共有 ────▶│ ndstool / grit / mmutil    │
│ フォルダ         │   (/work)    │ clangd（補完用）           │
└──────────────────┘              └────────────────────────────┘
```

- **ビルドはコンテナの中**でやります
- **`.nds` はホストのフォルダに出てきます**（フォルダを共有しているので）
- **melonDS はホストで動かします**（コンテナに画面はありません）

この構成の良いところは、**全員がまったく同じ環境になる**ことです。
「自分の PC だけビルドが通らない」が起きません。

---

## 12.2 事前に入れるもの

| ソフト | 用途 |
| --- | --- |
| [Docker Desktop](https://www.docker.com/products/docker-desktop/) | コンテナを動かす |
| [VS Code](https://code.visualstudio.com/) | エディタ |
| VS Code 拡張 [Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) | コンテナに接続する |
| [melonDS](https://melonds.kuribo64.net/) | DS エミュレータ |

Windows の人は Docker Desktop の **WSL2 バックエンド**を有効にしてください（既定で有効です）。

---

## 12.3 起動する

1. プロジェクトのフォルダを VS Code で開く
2. 右下に「**Reopen in Container**」と出るので押す
   - 出ない場合: `F1` → `Dev Containers: Reopen in Container`
3. **初回はイメージのビルドで 5〜15 分**かかります。コーヒーでも飲んでください
4. ターミナル（`` Ctrl+` ``）を開いて確認

```bash
$ whoami
dev
$ echo $BLOCKSDS
/opt/wonderful/thirdparty/blocksds/core
$ pwd
/work
```

こう出れば成功です。

> **コンテナの中では `$BLOCKSDS` を使ってください。**
> ネット上の記事には `/opt/blocksds/core` と書かれていることがありますが、
> このコンテナにそのパスはありません。環境変数の方が確実です。

---

## 12.4 サンプルをビルドしてみる

まず、既存のサンプルが通ることを確認します。

```bash
cp -r $BLOCKSDS/examples/graphics_2d/sprites_regular /work/test
cd /work/test
make
```

```
  GRIT    graphics/ball.png
  CC      source/main.c
  LD      build/sprites_basic.elf
  NDSTOOL sprites_basic.nds
```

`.nds` ができたら、**ホスト側のフォルダにも同じファイルが出ています。**
melonDS で開いてみてください。

サンプルの一覧はこれで見られます。

```bash
ls $BLOCKSDS/examples/
# audio  console  graphics_2d  graphics_3d  input  interrupts  ...
```

---

## 12.5 自分のプロジェクトを作る

**テンプレートをコピーする**のが公式の推奨手順です。

```bash
cp -r $BLOCKSDS/templates/rom_arm9_only/. /work/mygame/
cd /work/mygame
```

### ディレクトリ構成

```
mygame/
├── Makefile          ← 設定。触るのは上の方だけ
├── icon.gif          ← ROM のアイコン
├── source/           ← .c / .h をここに置く（自動で全部ビルドされる）
├── graphics/         ← .png + .grit を置くと C の配列に変換される
├── audio/            ← .wav / .mod を置くと maxmod 用に変換される
└── data/             ← .bin を置くと C の配列に変換される
```

### テンプレートのサンプル素材を消す

テンプレートには 3D とサウンドのデモが入っています。まず全部消します。

```bash
rm -rf graphics/* audio/* data/* source/*
```

**素材を消したら、`source/main.c` も作り直してください。**
元の `main.c` は消した素材のヘッダ（`neon.h` など）を `#include` しているので、
そのままだとビルドが通りません。

### Makefile で触る場所

上の「User config」だけです。下の方（ビルドルール）は触りません。

```makefile
NAME         := mygame            # 出力される .nds のファイル名
GAME_TITLE   := My Awesome Game   # DS のメニューに出るタイトル
GAME_SUBTITLE:= Summer Camp
GAME_AUTHOR  := Our Circle

SOURCEDIRS   := source            # ここ以下の .c を全部ビルド
COMPDB       := 1                 # ← 1 にする（補完のため。12.7 参照）

LIBS         := -lmm9 -lnds9      # デバッグ時は -lnds9d（11 章）
```

> **`source/` にファイルを追加しても、Makefile を編集する必要はありません。**
> `make` が勝手に拾います。サブフォルダを作っても大丈夫です。

---

## 12.6 Hello, DS

`source/main.c` を作ります。

```c
// source/main.c
#include <nds.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    defaultExceptionHandler();   // クラッシュ時に情報を出す（11 章）
    consoleDemoInit();           // 下画面をテキストコンソールにする

    printf("Hello, DS!\n\n");
    printf("A ボタン: カウント\n");
    printf("START:    終了\n");

    int count = 0;

    while (1)
    {
        scanKeys();                  // 入力を読む（毎フレーム 1 回だけ）
        u32 down = keysDown();

        if (down & KEY_A)     count++;
        if (down & KEY_START) break;

        swiWaitForVBlank();          // 次のフレームまで待つ（60fps に同期）

        consoleSetCursor(NULL, 0, 8);
        printf("count = %3d", count);
    }
    return 0;
}
```

```bash
make
```

ホスト側で `mygame.nds` を melonDS にドラッグ&ドロップ。
下画面に文字が出て、A ボタンでカウントが増えれば**成功です**。

### ビルドコマンド

```bash
make              # ビルド
make clean        # 中間ファイルと .nds を消す
make VERBOSE=1    # 実行しているコマンドを表示（トラブル時）
```

---

## 12.7 補完とジャンプを効かせる

VS Code の **clangd** が `#include <nds.h>` を解決できるようにします。
これがあると `oamSet` と打った時点で引数が出るので、**作業効率がまるで違います。**

**手順は 1 回だけ。**

1. Makefile の `COMPDB := 1` にする
2. `make` する → `compile_commands.json` ができる
3. clangd が自動で読み込む（`F1` → `clangd: Restart language server` で再読込）

`compile_commands.json` は「どのファイルをどのオプションでコンパイルしたか」の記録です。
clangd はこれを見て、libnds のヘッダの場所や `-D__NDS__` などを知ります。

**新しいライブラリを足したときや、Makefile を変えたときは `make` し直してください。**

---

## 12.8 コンテナの中身メモ

困ったときのために、どこに何があるかを書いておきます。

| もの | 場所 |
| --- | --- |
| プロジェクト（＝ホストの共有フォルダ） | `/work` |
| SDK | `$BLOCKSDS`（`/opt/wonderful/thirdparty/blocksds/core`） |
| **libnds のヘッダ** | `$BLOCKSDS/libs/libnds/include/nds/` |
| テンプレート | `$BLOCKSDS/templates/` |
| サンプル | `$BLOCKSDS/examples/` |
| ツール（ndstool, grit, mmutil…） | `$BLOCKSDS/tools/` |
| コンパイラ本体 | `$WONDERFUL_TOOLCHAIN/toolchain/gcc-arm-none-eabi/bin/` |

**libnds の使い方が分からないときは、ヘッダを直接読むのが一番早いです。**

```bash
grep -rn "oamSet" $BLOCKSDS/libs/libnds/include/nds/
```

Doxygen のコメントが丁寧に書かれているので、大抵これで解決します。

### コンパイラを直接呼びたいとき

`arm-none-eabi-gcc` は **PATH に入っていません**（Makefile がフルパスで呼んでいるため）。
`addr2line` などを手で使いたいときはパスを足してください。

```bash
export PATH=$WONDERFUL_TOOLCHAIN/toolchain/gcc-arm-none-eabi/bin:$PATH
arm-none-eabi-addr2line -e build/mygame.elf 0x020045A8
```

---

## 12.9 教科書のサンプルを動かす

[examples/](../examples/) にある C の文法サンプルは、**DS 用ではなく普通の `gcc`** で動きます。
そのためのコンパイラもコンテナに入れてあるので、そのまま実行できます。

```bash
cd /work/examples
./build.sh 04        # 04_ で始まるものをビルドして実行
```

> `gcc`（x86 用）と `arm-none-eabi-gcc`（DS 用）は別物です。
> **文法の実験は `gcc`、DS の ROM は `make`（中で `arm-none-eabi-gcc` が動く）** と覚えてください。

---

## 12.10 ライブラリを追加したくなったら

`Dockerfile` に足して、**コンテナを再ビルド**します
（`F1` → `Dev Containers: Rebuild Container`）。

```dockerfile
# BlocksDS のライブラリ（wf-pacman）
RUN wf-pacman -Syu --noconfirm && \
    wf-pacman -S --noconfirm \
        blocksds-default \
        blocksds-nitroengine \
    && \
    wf-config clean-caches --all

# Ubuntu 側のツール（apt）
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        clangd \
        python3 \
    && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/*
```

使えるライブラリの一覧はコンテナ内でこれを実行すると見られます。

```bash
wf-pacman -Ss blocksds
```

**Dockerfile を変えたら、必ずチーム全員に共有してください。**
再ビルドしないと他の人の環境ではビルドが通りません。

---

## 12.11 トラブルシューティング

| 症状 | 対処 |
| --- | --- |
| 「Reopen in Container」が出ない | `devcontainer.json` が `.devcontainer/` の中にあるか確認 |
| コンテナのビルドが遅い | 初回は 5〜15 分。2 回目以降はキャッシュが効いて数秒 |
| `Permission denied`（ファイルが作れない） | ホスト側のユーザー ID が 1000 か確認（`id -u`）。違うなら Dockerfile の `useradd -u 1000` を合わせる |
| `missing separator`（Makefile） | インデントがスペースになっている。**タブ**に直す |
| `neon.h: No such file or directory` | テンプレートの `graphics/` を消したのに `main.c` の `#include` が残っている |
| `undefined reference to 'mm...'` | maxmod を使うのに `LIBS` に `-lmm9` が無い |
| 補完が効かない / `nds.h` が赤い | `COMPDB := 1` にして `make`。その後 `F1` → `clangd: Restart language server` |
| ビルドは通るが melonDS で真っ白 | `defaultExceptionHandler()` を入れて、実は落ちていないか確認（11 章） |
| 変更が反映されない | `make clean && make` |

---

## 12.12 ここまでできたら

### 到達確認

- [ ] Dev Container が起動して `/work` にいる
- [ ] `make` が通って `.nds` ができる
- [ ] melonDS で起動して文字が出る
- [ ] A ボタンでカウントが増える
- [ ] `main.c` を書き換えて `make` すると表示が変わる
- [ ] `nds.h` の中の関数に補完が効く

**ここまでできれば、合宿の準備としては十分です。**

### 次に読むもの

1. この教科書の [1 章](01-mental-model.md) →[4 章](04-pointers.md) → [5 章](05-arrays-strings.md)
2. [BlocksDS 公式チュートリアル](https://blocksds.skylyrac.net/tutorial/)
   「Your first program」→「User input」→「Introduction to 2D graphics」
   →「Backgrounds」→「Sprites」の順が素直です

---

[← 11. デバッグ](11-debug.md) | [目次](../README.md) | [チートシート →](99-cheatsheet.md)
