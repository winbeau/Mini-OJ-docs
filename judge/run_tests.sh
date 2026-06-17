#!/usr/bin/env bash
# 跑全部示例提交,逐个打印判定结果(覆盖 8 种状态 + 多语言 + special)
set -u
S=examples/solutions
AB=examples/problems/aplusb
j() { printf "%-22s -> " "$1"; ./judge "${@:2}"; }

j "ac.cpp"             --problem $AB --src $S/ac.cpp  --lang cpp
j "wa.cpp"             --problem $AB --src $S/wa.cpp  --lang cpp
j "tle.cpp"            --problem $AB --src $S/tle.cpp --lang cpp
j "re.cpp"             --problem $AB --src $S/re.cpp  --lang cpp
j "ce.cpp"             --problem $AB --src $S/ce.cpp  --lang cpp
j "mle.cpp"            --problem $AB --src $S/mle.cpp --lang cpp --mem-mb 256
j "ac.py"              --problem $AB --src $S/ac.py   --lang python
j "pe.cpp(lines)"      --problem examples/problems/lines --src $S/pe.cpp  --lang cpp
j "avg.cpp(no-special)" --problem examples/problems/avg  --src $S/avg.cpp --lang cpp
j "avg.cpp(special)"   --problem examples/problems/avg   --src $S/avg.cpp --lang cpp --special
if command -v javac >/dev/null 2>&1; then
  j "ac.java"          --problem $AB --src $S/ac.java --lang java
else
  printf "%-22s -> (跳过: 无 JDK)\n" "ac.java"
fi
