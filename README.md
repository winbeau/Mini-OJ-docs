# Mini-OJ 学习文档

用一个 **Mini-OJ(在线判题系统)** 项目,循序渐进学完 Java 课程,并贯穿「判题机三代演进」主线(第13章网络已删,新增第15章泛型集合)。
环境:Ubuntu + nvim + JDK,**古法手撸**(纯 `javac/java` + `Makefile`),简单、教学优先。

> **在线教程**:<https://winbeau.github.io/Mini-OJ-docs/> ·(单页 HTML,全部 md 融合 + 目录/折叠/复制/暗色;本地文件 [mini-oj-tutorial.html](mini-oj-tutorial.html))
> **PDF 教程**(彩色,适合打印/离线):[考试速成版](mini-oj-exam-cram.pdf)(只看考试向,~8 页)· [完整版](mini-oj-tutorial.pdf)(考试+工程,~95 页)·(`build-pdf.sh` + tectonic 一键重编两本)

## 文档导航

| 文件 | 对应章节 | 里程碑 | 判题机代 |
|---|---|---|---|
| [00-项目总设计](00-项目总设计.md) | 全局契约(类清单 + 数据模型签名) | — | — |
| [01-Ch1-环境与HelloWorld](01-Ch1-环境与HelloWorld.md) | 第1章 | M0 | — |
| [02-Ch2-3-单题判题器](02-Ch2-3-单题判题器.md) | 第2-3章 | M1 | 一 |
| [03-Ch4-对象建模](03-Ch4-对象建模.md) | 第4章 | M2 | 一 |
| [04-Ch5-7-判题器与异常](04-Ch5-7-判题器与异常.md) | 第5-7章(接口/继承/多态/异常) | M3 | 一 |
| [05-M4-第二代判题机](05-M4-第二代判题机.md) | 第8章 + 第10章(单文件) | M4 | **二** |
| [06-M5a-第三代判题机](06-M5a-第三代判题机.md) | **第15章** + 第10章 + C++ | M5a | **三** |
| [07-M5b-数据库衔接](07-M5b-数据库衔接.md) | 第11章 | M5b | — |
| [08-M5c-Swing客户端](08-M5c-Swing客户端.md) | 第9章 | M5c | — |
| [09-M6a-多线程收官](09-M6a-多线程收官.md) | 第12章(收官) | M6a | 三 |

> 一页纸总路线图见 [mini-oj-plan.md](mini-oj-plan.md)(三代主线/里程碑/架构速览)。

## 判题机三代

- **第一代**(M1–M3):Java 对象在 JVM 内模拟判题(`Solution`+多态)。
- **第二代**(M4):反射工厂 + 单文件配置(`JudgeFactory`+`config.txt`)。
- **第三代**(M5a 起):独立 C++ 判题机真编译/真运行/真 `setrlimit`,代码在 [`judge/`](judge/)(单文件 `judge.cpp`);Java 用 `ProcessBuilder` 调用(`MachineJudge`)。

## 项目最终形态(无网络,本地大前端)

```
Swing 桌面客户端(Ch9) ──MVC──▶ 多线程判题队列(Ch12) ──调用──▶ C++ 判题机(judge)
        │                                                      真编译/真运行/真 setrlimit
        └──────────────▶ MySQL(Ch11,元数据/历史) ◀── ProblemService
                          文件系统:problems/<id>/{config,N.in,N.out} + submissions/ 源码
```

> 第13章网络(Socket)已删除:客户端不经网络,直接在同一 JVM 内以 MVC 调用本地的数据库与 C++ 判题代理。
