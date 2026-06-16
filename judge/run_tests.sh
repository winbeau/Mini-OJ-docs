#!/usr/bin/env bash
# 跑全部示例提交,逐个打印判定结果
set -u
P=examples/problems/aplusb
run() { printf "%-10s -> " "$2"; ./judge --problem "$P" --src "$1" --lang "$3" --time-ms 1000 --mem-mb 256; }

run examples/solutions/ac.cpp  "ac.cpp"  cpp
run examples/solutions/wa.cpp  "wa.cpp"  cpp
run examples/solutions/tle.cpp "tle.cpp" cpp
run examples/solutions/re.cpp  "re.cpp"  cpp
run examples/solutions/ce.cpp  "ce.cpp"  cpp
run examples/solutions/ac.py   "ac.py"   python
