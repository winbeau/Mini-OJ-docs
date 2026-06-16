# Mini-OJ 学习计划(用项目串起 Java 课程 13 章)

> 环境:Ubuntu + nvim + JDK。开发方式:先裸 `javac/java`,从单文件 → 多文件 → jar → 依赖管理,循序渐进。
> 主线:**给题目+测试用例,(编译)运行用户代码,比对输出,判 `AC/WA/TLE/MLE/CE/RE`。**
> 判题两条路:Java 内模拟判题(教学,M3/M4)+ 外部判题机真实编译运行 C++/Python(附录第11章)。

## 最终目标架构

```
Swing 客户端 ──Socket──▶ 多线程判题服务端 ──JDBC──▶ MySQL
   (Ch9)                  (Ch12/Ch13)              (Ch11)
                              │
                              ▼
                   判题核心:Problem/Judge/Result
                   (Ch2-8) + 文件题库(Ch10)
                              │
                              ▼
                   判题机(附录):编译运行 C++/Python 真实提交
```

## 里程碑总览

| 阶段 | 章节 | 里程碑 | 产物 |
|---|---|---|---|
| M0 | Ch1 | 环境 + HelloWorld | 能编译运行的 `Main.java` |
| M1 | Ch2–3 | 单题判题器(硬编码) | 能判 A+B 出 AC/WA 的单文件程序 |
| M2 | Ch4 | 对象建模 | `oj.core` 包 + 可运行 jar + javadoc |
| M3 | Ch5–7 | 多判题策略 + 健壮性 | `Judge` 接口/抽象类/多态 + 自定义异常 |
| M4 | Ch8 | 比对/解析增强 + 反射 | 正则 SpecialJudge + 反射可插拔判题器 |
| M5a | Ch10 | 文件题库 + 序列化 | 从 `problems/` 目录加载题、提交落盘 |
| M5b | Ch11 | JDBC + MySQL | `users/problems/submissions` 三表 + DAO + 事务 |
| M5c | Ch9 | Swing 桌面客户端 | 选题→编辑→提交→看结果(MVC) |
| M6a | Ch12 | 多线程判题队列 | 并发判题 + 守护线程做 TLE 超时 |
| M6b | Ch13 | 网络 C/S | Socket 客户端 ↔ ServerSocket 服务端 |
| 附录 | — | 判题机集成 | C++ 单文件判题机 + Java `ProcessBuilder` 调用(真实编译运行) |

## 项目目录(最终演进形态)

```
mini-oj/
├── src/oj/
│   ├── core/       Problem, TestCase, Submission, JudgeResult (M2)
│   ├── judge/      Judge, AbstractJudge, StandardJudge, SpecialJudge (M3-M4)
│   ├── exception/  JudgeException, TimeLimitException (M3)
│   ├── io/         题库加载、序列化 (M5a)
│   ├── db/         DAO + 连接/事务 (M5b)
│   ├── gui/        Swing 客户端 (M5c)
│   ├── net/        Socket 客户端/服务端 (M6)
│   └── Main.java
├── problems/       题库:title.txt / *.in / *.out (M5a)
├── lib/            mysql-connector-j.jar 等 (M5b)
├── build/          .class 输出
├── Makefile        编译/打包脚本 (M2 起)
├── judge/          判题机(C++ 单文件 judge.cpp,真实编译运行 C++/Python,见附录)
└── README.md
```

## 工具链演进

- **M0–M1**:`nvim` + `javac` + `java` + `javap`(反编译看字节码),单文件。
- **M2**:多文件 + `package`,`Makefile` 一键编译,`jar cfe` 打可运行包,`javadoc` 出文档。
- **M5b**:引入外部 jar(MySQL 驱动)→ 此处自然过渡到 **Maven/Gradle** 管依赖与 classpath。
- nvim:早期裸 javac 足够;中期可选装 `nvim-jdtls`(LSP)提升补全/跳转。

---

# 逐里程碑详细计划

每个里程碑统一四块:**知识点 / 任务(写什么)/ 关键命令 / 验收标准**。

## M0 — Ch1 环境与第一个程序

- **知识点**:Java 特点(了解)、JDK 安装与环境变量、编写→编译→运行三步、`javap` 反编译、编程风格(Allman/K&R)与注释。
- **任务**:装 JDK 并配环境;`nvim Main.java` 打印 `Mini-OJ judge ready`;建项目根目录 `mini-oj/`。
- **命令**:
  ```bash
  sudo apt install openjdk-21-jdk   # 或用 sdkman
  java -version && javac -version
  nvim Main.java
  javac Main.java && java Main
  javap -c Main                     # 看字节码,理解 .class/JVM
  ```
- **验收**:终端输出 ready;能讲清 `.java → .class → JVM`;能读懂一两行字节码。

## M1 — Ch2–3 单题判题器(硬编码)

- **知识点**:标识符/关键字、基本数据类型与类型转换、`Scanner` 输入/`println` 输出、数组(声明/分配/`length`/初始化/遍历)、算术/关系/逻辑/赋值/位运算、`if`/`switch`、`for`/`while`/`do-while`、`break`/`continue`。
- **任务**:
  1. 程序内置 N 组测试用例:`int[] a, b; int[] expected;`。
  2. 写"用户解法"方法 `solve(x,y)` 返回 `x+y`。
  3. `for` 逐组比对:相等记 AC,否则 WA 并打印"第 i 组 期望 X 实际 Y"。
  4. 再加一道用 `switch`(成绩等级)和一道用循环(判素数/求和)的题,练全流程控制。
- **验收**:能跑出 `3/3 AC` 或 `2/3 WA(...)`;故意改一组用例,判定正确翻转为 WA。

## M2 — Ch4 对象建模

- **知识点**:类/成员变量/方法、构造方法、对象创建与使用、引用 vs 实体、参数传值(基本/引用/可变参数)、对象组合、实例成员 vs 类成员(`static`)、方法重载、`this`、`package`/`import`、访问权限(private/public/友好/protected)、对象数组、可运行 jar、javadoc。
- **任务**:把 M1 拆成对象——
  - `TestCase{ String input; String expected; }`
  - `Problem{ int id; String title; TestCase[] cases; }`(**对象数组**;`addCases(TestCase... cs)` 用**可变参数**)
  - `Submission{ Problem problem; String userOutput; }`
  - `JudgeResult{ String status; String detail; int passed, total; }`
  - 全字段 `private` + getter;`static` 计数器统计总提交数;`judge(Problem)` / `judge(Problem,int)` **重载**;构造方法里用 `this`。
  - 放进 `package oj.core`,正确目录;写 `Makefile`。
- **命令**:
  ```bash
  javac -d build $(find src -name '*.java')
  jar cfe mini-oj.jar oj.Main -C build .
  java -jar mini-oj.jar
  javadoc -d doc -sourcepath src -subpackages oj
  ```
- **验收**:判题逻辑全用对象;`java -jar` 能跑;`doc/` 有 API 文档;能讲清"引用 vs 实体"。

## M3 — Ch5–7 多判题策略 + 健壮性

- **知识点**:继承/成员隐藏/方法重写/`super`/`final`/上转型/多态、抽象类与抽象方法、面向抽象、开闭原则;接口/实现/接口回调/函数接口+Lambda/接口多态/接口参数/抽象类 vs 接口/面向接口;内部类、匿名类、Lambda 代替匿名类、异常(`try-catch`、自定义异常)、断言。
- **任务**:
  1. 定义接口 `Judge{ JudgeResult judge(Problem p, Solution s); }`。
  2. `abstract AbstractJudge`:模板方法(遍历用例+统计),抽象 `boolean compare(String exp, String act)` 留子类。
  3. 子类:`StandardJudge`(精确,trim 行尾)、`SpecialJudge`(浮点容差 1e-6)、`TokenJudge`(忽略多余空白)。
  4. **多态**:`Judge j = pickJudge(p.type); j.judge(...)` —— 新增判题器不改调用方(**开闭原则**)。
  5. 自定义异常 `JudgeException`/`TimeLimitException`(见 00 契约);判题中 `try-catch` 把崩溃映射成 RE 状态(而非让程序挂掉)。
  6. 用 Lambda / 匿名类实现一个 `OutputComparator` 回调,体会两者关系。
  7. `assert passed <= total` 做内部不变量断言。
- **验收**:同一接口跑标准题与浮点题;造个会抛异常的解法→判 RE/CE;新增判题器只加类、不改老代码。

## M4 — Ch8 比对/解析增强 + 反射

- **知识点**:`String`(常用方法/与基本类型、字符数组、字节数组互转)、正则(元字符/替换/分解、`Pattern`/`Matcher`)、`StringTokenizer`、`Scanner`、`StringBuffer`、日期时间(`LocalDateTime`/`Duration`/格式化)、`Math`/`BigInteger`/`Random`/数字格式化、**反射(`Class`)**、`Arrays`/`System`。
- **任务**:
  1. 比对升级:`trim/split/equals` 做行级比对;`StringBuffer` 拼接判题报告。
  2. 正则:`SpecialJudge` 用 `Pattern`/`Matcher` 校验输出格式(如"若干空格分隔的整数"),提取数字。
  3. 解析:`Scanner`/`StringTokenizer` 切分输入数据。
  4. 日期:记录提交时间戳,`Duration` 统计判题耗时(为 TLE 铺垫)。
  5. `BigInteger`:加一道"大数加法/阶乘"题,验证高精度判题。
  6. **反射**:从配置字符串 `Class.forName("oj.judge.SpecialJudge").getDeclaredConstructor().newInstance()` 动态加载判题器 → 判题器可插拔。
- **验收**:浮点/格式题靠正则判对;报告含时间戳+耗时;改配置字符串就能换判题器。

## M5a — Ch10 文件题库 + 序列化

- **知识点**:`File`(属性/目录/创建删除)、字节流、字符流、缓冲流、随机流(`RandomAccessFile`)、数据流(`DataOutputStream`)、对象流、序列化、`Scanner` 解析文件、(选)文件锁。
- **任务**:
  1. 题库落地:`problems/1/title.txt`、`problems/1/1.in`、`problems/1/1.out`;用 `File` 遍历目录加载题与用例,**替换硬编码**。
  2. 字符流+缓冲流读 `.in/.out`;`Scanner` 解析文件。
  3. `Submission implements Serializable`,对象流写到 `submissions/*.ser`,反序列化读回历史。
  4. 数据流写紧凑统计;`RandomAccessFile` 追加判题日志;(选)文件锁保护并发写(为多线程铺垫)。
- **验收**:加新题只需往 `problems/` 放文件、不改代码;重启后能从 `.ser` 读回历史提交。

## M5b — Ch11 JDBC + MySQL

- **知识点**:MySQL 启动/客户端、JDBC、连接、查询(顺序/游标/条件排序)、增删改、`PreparedStatement`+通配符、通用查询、事务。
- **任务**:
  1. 装 MySQL,建库 `mini_oj`,建表 `users / problems / submissions`。
  2. 引入 `mysql-connector-j.jar`(放 `lib/`)→ **此处引入 Maven/Gradle 管依赖**。
  3. DAO:`ProblemDao`/`SubmissionDao` 用 `PreparedStatement` 做 CRUD;查历史提交、排行。
  4. **事务**:一次提交 = 插入 submission + 更新 user 通过题数,放进一个事务(`setAutoCommit(false)` + `commit/rollback`)。
  5. 元数据/提交从文件平滑迁到 DB(题面可仍放文件)。
- **验收**:提交进 MySQL;能按用户/题目查询;事务中途抛异常时统计不被脏写(回滚生效)。

## M5c — Ch9 Swing 桌面客户端

- **知识点**:Swing 概述、`JFrame`、菜单、常用组件/容器/布局、事件处理(Action/Item/Document/Mouse/Key/Window/Focus)、匿名类/Lambda 监听器、MVC、对话框、`JTree`、`JTable`、按钮绑键盘。
- **任务**:
  1. 主窗:左 `JTree`/`JList` 题目列表(读 DB),中 `JTextArea` 代码/输入编辑区,下 `JTable` 显示各用例 AC/WA + 耗时。
  2. 菜单条(文件/帮助)+ "提交"按钮。
  3. 事件:点题目→加载题面;点提交→判题→刷新结果表(Lambda 监听器);至少用到 3 类事件。
  4. 对话框:结果消息框、异常错误框、退出确认框。
  5. **MVC** 分层:Model(DAO+判题)/ View(Swing)/ Controller(监听器)。
- **验收**:纯 GUI 完成"选题→编辑→提交→看结果";结构是 MVC。

## M6a — Ch12 多线程判题队列

- **知识点**:进程/线程、状态生命周期、调度优先级、`Thread`/`Runnable`、`run` 调用次数、常用方法、`synchronized` 同步、`wait/notify` 协调、`join` 联合、GUI 线程(`SwingWorker`/EDT)、计时器线程、守护线程。
- **任务**:
  1. 判题队列:提交进 `BlockingQueue`(或自写 synchronized 队列),N 个工作线程消费(生产者-消费者)。
  2. 同步保护共享统计;`wait/notify` 协调空队列。
  3. **TLE**:每组用例起一个守护/计时线程,超时(如 1s)→中断→判 TLE(呼应 M4 的 `Duration`)。
  4. GUI 判题放后台 `SwingWorker`,结果回 EDT 刷新表格,界面不卡。
  5. `join` 批量判题等全部完成再汇总。
- **验收**:多份提交并发判;死循环解法被判 TLE 而非卡死;判题时 GUI 不冻结。

## M6b — Ch13 网络 C/S(收官)

- **知识点**:`URL`(构造/读资源)、`InetAddress`(地址表示/获取)、套接字、`Socket`/`ServerSocket`。
- **任务**:
  1. 拆 C/S:服务端 `ServerSocket` 监听,收到提交丢进 M6a 判题队列,判完把结果写回 `Socket`。
  2. 客户端(Swing)用 `Socket` 发"题号+代码/输出",收结果显示。
  3. 服务端多线程:每连接一个线程或线程池(呼应 Ch12)。
  4. `InetAddress` 显示客户端来源;(选)`URL` 从远程拉题库。
  5. 最终形态成立:**Swing 客户端 ↔ Socket ↔ 多线程判题服务端 ↔ MySQL**。
- **验收**:两进程(同机不同端口)跑通"远程提交→远程判题→结果回显";多客户端并发不串。

---

## 学习方法建议

- **一里程碑一可运行产物**:每个 M 结束都能 `java`/`java -jar` 跑出东西,再进下一个。
- **先理解再扩展**:每章新知识先用在 OJ 的某个真实需求上,避免"为语法而语法"。
- **保留旧版本**:每个里程碑提交一次(可 `git init`),方便回看演进。
- **验收当门槛**:没过验收标准不进下一阶段。

> 进度勾选:M0 ☐  M1 ☐  M2 ☐  M3 ☐  M4 ☐  M5a ☐  M5b ☐  M5c ☐  M6a ☐  M6b ☐
