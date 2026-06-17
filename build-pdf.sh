#!/usr/bin/env bash
# 从各 md 生成单文件 PDF 教程(pandoc + tectonic + ctexart)。
# 依赖:pandoc、tectonic(单文件静态 XeTeX 引擎,首跑联网拉宏包)、CJK 字体 WenQuanYi Zen Hei。
# 用法:cd mini-oj-docs && TECTONIC=/路径/tectonic bash build-pdf.sh
#   产出:mini-oj-tutorial.tex(可重编源)+ mini-oj-tutorial.pdf
set -e
export LANG=C.UTF-8 LC_ALL=C.UTF-8
BASE="$(cd "$(dirname "$0")" && pwd)"
TECTONIC="${TECTONIC:-tectonic}"
TMP="$(mktemp -d)"

# 按教程顺序拷成 ASCII 名(规避 pandoc 对中文文件名的 locale 解码问题)
FILES=("mini-oj-plan.md" "00-项目总设计.md" "01-Ch1-环境与HelloWorld.md" "02-Ch2-3-单题判题器.md" \
       "03-Ch4-对象建模.md" "04-Ch5-7-判题器与异常.md" "05-M4-第二代判题机.md" "06-M5a-第三代判题机.md" \
       "07-M5b-数据库衔接.md" "08-M5c-Swing客户端.md" "09-M6a-多线程收官.md" "judge/README.md")
i=1
for f in "${FILES[@]}"; do cp "$BASE/$f" "$(printf '%s/p%02d.md' "$TMP" "$i")"; i=$((i+1)); done

# 制表符/箭头 → ASCII(拉丁等宽字体无这些字形)
python3 - "$TMP" <<'PY'
import glob, sys
rep={'─':'-','│':'|','┌':'+','┐':'+','└':'+','┘':'+','├':'+','┤':'+','┬':'+','┴':'+','┼':'+','━':'-','┃':'|',
     '▶':'>','◀':'<','▼':'v','▲':'^','→':'->','←':'<-','↓':'|','↑':'|','⬇':'v','►':'>','▸':'>','▷':'>','◆':'*','◇':'*','●':'*','■':'*'}
for f in glob.glob(sys.argv[1]+'/p*.md'):
    s=open(f,encoding='utf-8').read()
    for k,v in rep.items(): s=s.replace(k,v)
    open(f,'w',encoding='utf-8').write(s)
PY

# 符号字形(★☐①…)映射到 Dingbats/amssymb
cat > "$TMP/header.tex" <<'EOF'
\usepackage{fvextra}
\DefineVerbatimEnvironment{Highlighting}{Verbatim}{breaklines,breakanywhere,fontsize=\small,commandchars=\\\{\}}
\usepackage{pifont}
\usepackage{amssymb}
\usepackage{newunicodechar}
\newunicodechar{★}{\ding{72}}
\newunicodechar{☆}{\ding{73}}
\newunicodechar{✘}{\ding{55}}
\newunicodechar{✓}{\ding{51}}
\newunicodechar{✔}{\ding{51}}
\newunicodechar{☐}{$\square$}
\newunicodechar{①}{\ding{172}}
\newunicodechar{②}{\ding{173}}
\newunicodechar{③}{\ding{174}}
\newunicodechar{④}{\ding{175}}
\newunicodechar{⑤}{\ding{176}}
\newunicodechar{⑥}{\ding{177}}
\setlength{\parindent}{0pt}
\setlength{\parskip}{4pt}
EOF

pandoc "$TMP"/p*.md -f gfm -s --toc --toc-depth=2 -H "$TMP/header.tex" \
  -V documentclass=ctexart -V CJKmainfont='WenQuanYi Zen Hei' \
  -V geometry:margin=2.3cm -V fontsize=11pt -V colorlinks=true -V linkcolor=NavyBlue -V urlcolor=NavyBlue \
  -V title="用 Mini-OJ 项目学 Java —— 判题机三代演进" \
  -V author="winbeau · Mini-OJ 教学项目" -V date="2026-06" \
  --highlight-style=tango -o "$BASE/mini-oj-tutorial.tex"

"$TECTONIC" -X compile "$BASE/mini-oj-tutorial.tex"
rm -rf "$TMP"
echo "done -> mini-oj-tutorial.pdf"
