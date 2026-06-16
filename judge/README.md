# Mini-OJ 判题机 (judge)

单文件、编译型的判题机。**编译并运行**用户提交(C++ / Python),跑全部测试点,
带 CPU / 墙钟 / 内存限制,比对输出,输出一行 **JSON** 判定。Java OJ 只需调用它并解析结果。

## 编译

```bash
make            # 等价于 g++ -O2 -std=c++17 -o judge judge.cpp
```

## 用法

```bash
./judge --problem <题目目录> --src <提交源码> --lang <cpp|python> \
        [--time-ms 1000] [--mem-mb 256]
```

题目目录约定(成对、按数字排序):

```
problems/aplusb/
├── 1.in   1.out
├── 2.in   2.out
└── 3.in   3.out
```

## 输出(stdout 一行 JSON)

```json
{"status":"AC","passed":3,"total":3,"time_ms":3,"mem_kb":1840,"detail":""}
```

| 字段 | 含义 |
|---|---|
| `status` | `AC`/`WA`/`TLE`/`MLE`/`RE`/`CE`/`ERR` |
| `passed` / `total` | 通过测试点数 / 总数 |
| `time_ms` | 最大测试点 CPU 耗时 |
| `mem_kb` | 峰值内存(ru_maxrss) |
| `detail` | 失败说明(WA/RE 的 case 号、CE 的编译错误等) |

状态含义:AC 通过 · WA 答案错 · TLE 超时 · MLE 超内存 · RE 运行错(崩溃/非零退出) · CE 编译/语法错 · ERR 判题机内部错。

## 例子

```bash
./judge --problem examples/problems/aplusb --src examples/solutions/ac.cpp  --lang cpp
./judge --problem examples/problems/aplusb --src examples/solutions/ac.py   --lang python
make test        # 跑全部示例提交,逐个打印判定
```

## Java OJ 如何调用(示意)

```java
Process p = new ProcessBuilder(
    "./judge", "--problem", "problems/1", "--src", "sub.cpp", "--lang", "cpp"
).start();
String json = new String(p.getInputStream().readAllBytes());   // 解析 status/passed/...
```

## 限制与边界(教学版)

- 资源限制用 `setrlimit`(CPU/AS/输出) + 父进程墙钟看门狗,**未做完整沙箱**(无 seccomp/cgroups、不禁系统调用、不隔离文件系统)。**只在你信任的本地/单机环境运行提交。**
- 内存判定以 `ru_maxrss` 为准;`RLIMIT_AS` 仅作"防吃光内存"的安全上限(给到限额 2 倍余量),避免正常程序因虚拟内存预留被误判 MLE。
- 输出比对:逐行去行尾空白、忽略末尾空行(`normalize`)。需要更严或 special judge 可再扩展。
