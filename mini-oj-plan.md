# Mini-OJ 学习路线图(判题机三代演进)

> 环境:Ubuntu + nvim + JDK,**古法手撸**(纯 `javac/java` + `Makefile`),简单、教学优先。
> 主线:用一个 Mini-OJ 串起 Java 课程,并贯穿一条**判题机三代演进**。
> 课程调整:**第13章网络(Socket)不考,已删除**;**新增第15章泛型与集合**(并入 M5a)。

## 判题机三代主线

| 代 | 里程碑 | 判题方式 | 关键技术 |
|---|---|---|---|
| **第一代** | M1–M3(已完成) | Java 对象在 JVM 内模拟 | `Solution.solve()` + 多态 |
| **第二代** | M4 | 反射工厂 + 单文件配置 | `Class.forName` + `config.txt` |
| **第三代** | M5a | 外部 C++ 判题机真编译真运行 | `ProcessBuilder` + `setrlimit` |

## 考试相关度(★ = 历年真题强相关)

依据新疆大学《面向对象程序设计 B / JAVA 程序设计》历年真题(2016-2017、2019-2020 A 卷 + 复习提纲、笔试模拟)。题型固定:**单选 + 判断 + 读程序 + 程序填空 + 编程**,各约 20 分。

| 模块(章) | ★相关度 | 真题题型(原题级) | 里程碑 |
|---|---|---|---|
| 类与对象/构造/封装/static/this(Ch4) | ★★★ | 编程大题(Vehicle 类)、读程序、选择/判断 | M2 |
| 继承/重写/多态/抽象类/接口/异常(Ch5-7) | ★★★ | 编程大题(抽象类 Shape→Circle/Rectangle)、读程序、选择 | M3 |
| Swing GUI + 事件(Ch9) | ★★★ | 编程(平方/按钮窗口)+ 填空(六按钮 GridLayout)双大题、选择 | M5c |
| 集合 ArrayList/LinkedList/HashMap(Ch15) | ★★ | 读程序(Iterator)、填空(查询入 ArrayList) | M5a |
| JDBC 查询/更新(Ch11) | ★★ | 填空(查询入 List)、原题(score_t / student 表) | M5b |
| 多线程 Thread/Runnable/join/synchronized(Ch12) | ★★ | 填空(线程类)、选择 | M6a |
| String/StringBuffer/Random/Math(Ch8) | ★★ | 读程序(`==`/equals)、选择 | M4 |
| 数组/运算符/流程(Ch2-3) | ★ | 选择、读程序 | M1 |
| 异常 try-catch(Ch7) | ★ | 选择、判断 | M3 |
| 文件 File/流/序列化(Ch10) | ★ | 少量选择/判断(File.mkdir、System.in.read、对象串行化) | M4/M5a |
| 环境/编译运行(Ch1) | ☆ | 少量选择 | M0 |
| 网络 Socket(Ch13) | ✘ 已删 | 历年仅 1 道判断 | — |

> **工程化加料**(反射工厂、外部 C++ 判题机、ProcessBuilder、ProblemService/泛型架构、线程池/队列、MVC 解耦)**不是考点**,属项目深度。

## 两步走(每个里程碑先「拟合试卷」再「工程化」)

本项目偏工程化。为让学生**先考得了、上得了手**,每个里程碑拆成两步:

1. **第一步 [preliminary · 拟合试卷]**:按真题难度做最小可运行版(教材/考试写法),快速有参与感、直接覆盖考点。
2. **第二步 [工程化]**:在此基础上引入反射 / 外部 C++ 判题机 / 泛型架构 / 数据库 / 线程池 / MVC 解耦等工程方法。

> 各章文档开头已标 `★相关度` 与 `[第一步]/[第二步]` 的具体内容。**考前只过第一步即可覆盖真题**;想做工程深度再上第二步。

## 里程碑总览

| 里程碑 | 章节 | 主题 | ★ | 代 | 产物 |
|---|---|---|---|---|---|
| M0 | Ch1 | 环境 + HelloWorld | ☆ | — | 能编译运行的 `Main.java` |
| M1 | Ch2–3 | 单题判题器(硬编码) | ★ | 一 | 数组+流程控制判 A+B |
| M2 | Ch4 | 对象建模 | ★★★ | 一 | `oj.core` 包 + jar + javadoc |
| M3 | Ch5–7 | 接口/继承/多态/异常 | ★★★ | 一 | `Judge` 接口 + `AbstractJudge` + 子类 |
| M4 | Ch8 + Ch10(单文件) | 第二代:反射 + 单文件配置 | ★★ Ch8 / 工程 反射 | 二 | `ConfigFile`/`SingleFileProblemLoader`/`JudgeFactory` |
| M5a | Ch15 + Ch10(工业级) | 第三代:泛型集合 + C++ 判题机 | ★★ Ch15 / 工程 C++ | 三 | `ProblemLoader(List)`/`ProblemRepository`/`MachineJudge` |
| M5b | Ch11 | 数据库衔接(FS/DB 分工) | ★★ | — | `Db`/`ProblemDao`/`SubmissionDao`/`ProblemService` |
| M5c | Ch9 | Swing 大前端(MVC,无网络) | ★★★ | — | `OjFrame`/`OjController` |
| M6a | Ch12 | 多线程并发判题(收官) | ★★ 线程 / 工程 队列 | 三 | `JudgeTask`/`JudgeQueue`/`JudgeWorker` |

## 最终架构(无网络,本地大前端)

```
        Swing 桌面客户端(Ch9)   ——MVC——
          │ 选题/选语言/贴源码          │
          ▼                            ▼
   多线程判题队列(Ch12) ──调用──▶ C++ 判题机(judge)
          │                       真编译/真运行/真 setrlimit
          ▼                            │ 一行 JSON
     MySQL(Ch11)  ◀──元数据/历史──── ProblemService
          ▲                            │
          └── 文件系统:problems/<id>/{config,N.in,N.out} + submissions/ 源码
```

## 项目目录(最终形态)

```
mini-oj/
├── src/oj/
│   ├── core/       Status TestCase ProblemMeta Problem JudgeResult Submission JudgeTask
│   ├── judge/      Solution Judge AbstractJudge StandardJudge SpecialJudge
│   │   │           JudgeFactory(M4) MachineJudge(M5a)
│   │   └── queue/  JudgeQueue JudgeWorker (M6a)
│   ├── exception/  JudgeException TimeLimitException (M3)
│   ├── io/         ConfigFile SingleFileProblemLoader (M4)
│   │               ProblemLoader ProblemRepository SubmissionStore (M5a)
│   ├── db/         Db ProblemDao SubmissionDao (M5b)
│   ├── service/    ProblemService (M5b)
│   ├── gui/        OjFrame OjController (M5c)
│   └── Main.java
├── problems/<id>/  config.txt + N.in/N.out
├── submissions/    选手源码落地
├── judge/          C++ 判题机(judge.cpp,编译型)
├── lib/            mysql-connector-j.jar (M5b)
└── Makefile
```

## 工具链演进

- **M0–M1**:`nvim` + `javac` + `java` + `javap`,单文件。
- **M2**:多文件 + `package`,`Makefile`,`jar`,`javadoc`。
- **M4**:`File`/`BufferedReader` 读 `config.txt`;`g++` 预编译出 `judge` 二进制(M5a 起调用)。
- **M5b**:引入 MySQL 驱动 jar(`java -cp build:lib/*`)。

## 逐里程碑要点(★相关度 · 第一步拟合试卷 / 第二步工程化)

- **M0 Ch1** ☆:走通编写→编译→运行;`Main` 打印就绪。
- **M1 Ch2–3** ★:数组+if/switch/循环,硬编码判 A+B 出 AC/WA(真题:数组遍历、`++i`、运算符)。
- **M2 Ch4** ★★★:`[第一步]` 照真题写带构造/get-set 的普通类(如 Vehicle);`[第二步]` 收成 `Problem/TestCase` + static 计数 + 包/jar。
- **M3 Ch5–7** ★★★:`[第一步]` 照真题写抽象基类+子类重写+多态+try-catch(如 Shape→Circle/Rectangle);`[第二步]` 抽成 `Judge` 接口 / `AbstractJudge` 模板。
- **M4 Ch8+Ch10** ★★(Ch8):`[第一步]` String `==`/equals、StringBuffer、Random/Math(读程序/选择);`[第二步·工程]` `config.txt`+`Class.forName` 反射工厂、单文件读写。
- **M5a Ch15+Ch10** ★★(Ch15):`[第一步]` ArrayList/LinkedList/HashMap/Iterator 遍历(读程序/填空);`[第二步·工程]` `ProcessBuilder` 调外部 C++ 判题机、序列化、`ProblemRepository`。
- **M5b Ch11** ★★:`[第一步]` 照真题写 Connection/Statement/ResultSet 查询入 ArrayList + 更新(score_t/student 原题);`[第二步·工程]` `ProblemDao`/事务/`ProblemService` 的 FS/DB 分工。
- **M5c Ch9** ★★★:`[第一步]` 照真题写「加法/平方/六按钮」窗口(JFrame+布局+JButton+JTextField+JLabel+ActionListener);`[第二步·工程]` `OjFrame`/`OjController` MVC 解耦 + SwingWorker。
- **M6a Ch12** ★★:`[第一步]` 照真题写 `extends Thread`/`Runnable` + start/run + join/synchronized(线程类填空原题);`[第二步·工程]` 泛型阻塞队列 + 工作线程池 + 调 C++ 判题机(**收官**)。

## 学习方法

- 一里程碑一可运行产物,过了验收再进下一阶段。
- 每章新知识先用在 OJ 的真实需求上。
- 每个里程碑 `git` 提交一次,方便回看演进。

> 进度:M0 ☐  M1 ☐  M2 ☐  M3 ☐  M4 ☐  M5a ☐  M5b ☐  M5c ☐  M6a ☐
