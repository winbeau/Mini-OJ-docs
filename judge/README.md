# Mini-OJ 第三代判题机 (judge)

单文件、编译型的判题机:**真编译、真运行、真限资源**,跑全部测试点,比对输出,输出一行 **JSON** 判定。
是 Mini-OJ「判题机三代演进」里的**第三代**;Java OJ(`MachineJudge`)只负责 `ProcessBuilder` 调用它 + 正则解析结果。

## 能力一览

- **多语言**:`cpp` / `c` / `python` / `java`(自动编译 / 语法检查 / 构造运行命令)。
- **完整判定**:`AC / WA / PE / TLE / MLE / RE / CE / ERR`。
- **资源限制**:`setrlimit`(CPU/AS/输出/栈)+ 父进程墙钟看门狗 + **进程组整组 SIGKILL**(连同子孙进程一起掐);死循环必出 `TLE`、爆内存必出 `MLE`。
- **编译保护**:编译也有墙钟上限(g++ 10s / javac 15s),防模板炸弹挂死。
- **比对**:逐行去尾空白 + 去末尾空行;`--special` 开浮点容差(1e-6,逐 token);仅空白排布不同判 `PE`。
- **健壮**:缺文件 / 无用例 / 编译器缺失 / 异常输出 都有明确状态,绝不崩。

## 编译

```bash
make            # 等价于 g++ -O2 -std=c++17 -o judge judge.cpp
```

## 用法

```bash
./judge --problem <题目目录> --src <提交源码> --lang <cpp|c|python|java> \
        [--time-ms 1000] [--mem-mb 256] [--special]
# --time / --mem 是 --time-ms / --mem-mb 的别名
```

题目目录约定(成对、按数字排序;非 `.in/.out` 文件忽略):

```
problems/<id>/
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
| `status` | `AC`/`WA`/`PE`/`TLE`/`MLE`/`RE`/`CE`/`ERR` |
| `passed` / `total` | 通过测试点数 / 总数 |
| `time_ms` | 最大测试点 CPU 耗时 |
| `mem_kb` | 峰值内存(`ru_maxrss`) |
| `detail` | 失败说明(WA/PE/RE 的 case 号、CE 的编译错误等) |

状态:AC 通过 · WA 答案错 · **PE 格式错(仅空白差异)** · TLE 超时 · MLE 超内存 · RE 运行错(崩溃/非零退出) · CE 编译/语法错 · ERR 判题机内部错。

## 例子

```bash
make test       # 跑全部示例:AC/WA/TLE/RE/CE/MLE/PE + python + 浮点 special
./judge --problem examples/problems/aplusb --src examples/solutions/ac.cpp  --lang cpp
./judge --problem examples/problems/avg    --src examples/solutions/avg.cpp --lang cpp --special
```

## 放置位置(集成进 Mini-OJ 项目)

```
mini-oj/
├── src/oj/...           # Java 代码(MachineJudge 在 oj.judge)
├── problems/<id>/       # 题目:config.txt + N.in/N.out
└── judge/
    ├── judge.cpp
    └── judge            # make 出来的二进制(.gitignore)
```

## Java OJ 如何调用(第三代核心:ProcessBuilder + 正则解析)

```java
// oj.judge.MachineJudge
Process p = new ProcessBuilder(
    "judge/judge", "--problem", "problems/1", "--src", "sub.cpp", "--lang", "cpp",
    "--time-ms", "1000", "--mem-mb", "256"
).start();
String json = new String(p.getInputStream().readAllBytes(), StandardCharsets.UTF_8);
p.waitFor();
// 用正则从 json 抽 status/passed/total/time_ms,构造 oj.core.JudgeResult
```
完整封装见教程 **M5a · 第三代判题机** 一章的 `MachineJudge`。

## 限制与边界(教学/单机版)

- 用 `setrlimit` + 进程组隔离,但**未做完整沙箱**(无 seccomp/namespace/chroot、不禁系统调用、不隔离文件系统)。**只在你信任的本地/实验室环境运行提交。**
- 内存判定以 `ru_maxrss` 为准;`RLIMIT_AS` 只对 `c/cpp` 设硬上限(`java`/`python` 解释器预留巨量虚拟内存,设 AS 会误杀)。
- CPU 时限到点可能由内核以 `SIGXCPU` 或硬限 `SIGKILL` 触发,判题机据"CPU 已超限"统一归 `TLE`。
- `java` 提交的 public 类名须为 `Main`(判题机会把源码复制成 `Main.java` 再 `javac`)。
