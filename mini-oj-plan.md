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

## 里程碑总览

| 里程碑 | 章节 | 主题 | 代 | 产物 |
|---|---|---|---|---|
| M0 | Ch1 | 环境 + HelloWorld | — | 能编译运行的 `Main.java` |
| M1 | Ch2–3 | 单题判题器(硬编码) | 一 | 数组+流程控制判 A+B |
| M2 | Ch4 | 对象建模 | 一 | `oj.core` 包 + jar + javadoc |
| M3 | Ch5–7 | 接口/继承/多态/异常 | 一 | `Judge` 接口 + `AbstractJudge` + 子类 |
| M4 | Ch8 + Ch10(单文件) | **第二代:反射 + 单文件配置** | 二 | `ConfigFile`/`SingleFileProblemLoader`/`JudgeFactory` |
| M5a | **Ch15** + Ch10(工业级) | **第三代:泛型集合 + C++ 判题机** | 三 | `ProblemLoader(List)`/`ProblemRepository`/`MachineJudge` |
| M5b | Ch11 | 数据库衔接(FS/DB 分工) | — | `Db`/`ProblemDao`/`SubmissionDao`/`ProblemService` |
| M5c | Ch9 | Swing 大前端(MVC,无网络) | — | `OjFrame`/`OjController` |
| M6a | Ch12 | 多线程并发判题(收官) | 三 | `JudgeTask`/`JudgeQueue`/`JudgeWorker` |

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

## 逐里程碑要点(痛点 → 产物)

- **M0 Ch1**:走通编写→编译→运行;`Main` 打印就绪。
- **M1 Ch2–3**:数组+if/switch/循环,硬编码判 A+B 出 AC/WA。
- **M2 Ch4**:封装/构造/static/包/jar,把散数据收成对象。
- **M3 Ch5–7**:`Judge` 接口 + `AbstractJudge` 模板 + 多态 + 自定义异常(第一代成型)。
- **M4 Ch8+Ch10**:痛点=硬编码、判题器写死 → `config.txt` 配判题类名 + 反射造判题器 + 读单文件用例(**第二代**)。
- **M5a Ch15+Ch10**:痛点=JVM 限不住 CPU/内存、启动慢 → 泛型集合(`List`/`HashMap`/Stream)管多用例 + `ProcessBuilder` 调外部 C++ 判题机真判(**第三代,重大跨越**)。
- **M5b Ch11**:痛点=纯文件难统计 → 元数据/历史进 MySQL(事务),`.in/.out` 与源码留 FS;`ProblemService` 合流。
- **M5c Ch9**:痛点=无界面 → Swing 选题/选语言/贴源码/看结果,MVC 直连本地 DB 与 C++ 判题代理(无网络)。
- **M6a Ch12**:痛点=排队阻塞、死循环卡死 → 泛型阻塞队列 + N 个守护工作线程并发判,C++ 掐断 TLE,`SwingWorker` 不卡界面(**收官**)。

## 学习方法

- 一里程碑一可运行产物,过了验收再进下一阶段。
- 每章新知识先用在 OJ 的真实需求上。
- 每个里程碑 `git` 提交一次,方便回看演进。

> 进度:M0 ☐  M1 ☐  M2 ☐  M3 ☐  M4 ☐  M5a ☐  M5b ☐  M5c ☐  M6a ☐
