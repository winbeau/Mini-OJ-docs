#!/usr/bin/env bash
# 生成两本彩色 PDF:完整版(考试+工程)+ 考试速成版。
# 依赖:pandoc、tectonic(单文件静态 XeTeX,首跑联网拉宏包)、CJK 字体 WenQuanYi Zen Hei。
# 用法:cd mini-oj-docs && TECTONIC=/路径/tectonic bash build-pdf.sh
set -e
export LANG=C.UTF-8 LC_ALL=C.UTF-8
BASE="$(cd "$(dirname "$0")" && pwd)"
TECTONIC="${TECTONIC:-tectonic}"

preprocess() {  # $1 = 目录:制表符→ASCII;考试向/工程向→\KS/\GC 徽章
  python3 - "$1" <<'PY'
import glob, sys
sym={'─':'-','│':'|','┌':'+','┐':'+','└':'+','┘':'+','├':'+','┤':'+','┬':'+','┴':'+','┼':'+','━':'-','┃':'|',
     '▶':'>','◀':'<','▼':'v','▲':'^','→':'->','←':'<-','↓':'|','↑':'|','⬇':'v','►':'>','▸':'>','▷':'>','◆':'*','◇':'*','●':'*','■':'*'}
badge=[('**[第一步·考试向]**','**[第一步]** \\KS'),('**[第二步·工程向]**','**[第二步]** \\GC'),
       ('[考试向]','\\KS'),('[工程向]','\\GC')]
for f in glob.glob(sys.argv[1]+'/p*.md'):
    s=open(f,encoding='utf-8').read()
    for k,v in sym.items():   s=s.replace(k,v)
    for k,v in badge:         s=s.replace(k,v)
    open(f,'w',encoding='utf-8').write(s)
PY
}

build() {  # $1 输出名  $2 主标题  $3 副标题  其余=文档列表
  local out="$1" tmain="$2" tsub="$3"; shift 3
  local TMP; TMP="$(mktemp -d)"; local i=1
  for f in "$@"; do cp "$BASE/$f" "$(printf '%s/p%02d.md' "$TMP" "$i")"; i=$((i+1)); done
  preprocess "$TMP"
  printf '\\def\\TitleMain{%s}\n\\def\\TitleSub{%s}\n\\input{%s/pdf-preamble.tex}\n' "$tmain" "$tsub" "$BASE" > "$TMP/head.tex"
  pandoc "$TMP"/p*.md -f markdown-tex_math_dollars-tex_math_single_backslash -s --toc --toc-depth=2 -H "$TMP/head.tex" \
    -V documentclass=ctexart -V CJKmainfont='WenQuanYi Zen Hei' \
    -V geometry:margin=2.3cm -V fontsize=11pt \
    -V title="$tmain" -V author="winbeau" -V date="2026-06" \
    --highlight-style=tango -o "$BASE/$out.tex"
  "$TECTONIC" -X compile "$BASE/$out.tex"
  rm -rf "$TMP"
  echo "done -> $out.pdf"
}

# 完整版(考试 + 工程)
build "mini-oj-tutorial" "用 Mini-OJ 项目学 Java" "判题机三代演进 · 完整版(考试 + 工程)" \
  "mini-oj-plan.md" "00-项目总设计.md" "01-Ch1-环境与HelloWorld.md" "02-Ch2-3-单题判题器.md" \
  "03-Ch4-对象建模.md" "04-Ch5-7-判题器与异常.md" "05-M4-第二代判题机.md" "06-M5a-第三代判题机.md" \
  "07-M5b-数据库衔接.md" "08-M5c-Swing客户端.md" "09-M6a-多线程收官.md" "judge/README.md"

# 考试速成版(只考试向)
build "mini-oj-exam-cram" "Mini-OJ · Java 考试速成" "拟合试卷 · 只看考试向" "exam-cram.md"
