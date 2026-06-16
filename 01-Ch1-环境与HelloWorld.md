# 01 · 第1章 环境与 HelloWorld(里程碑 M0)

> 目标:装好 JDK,理解"编写→编译→运行",跑出第一个程序。**本章只有一个文件、一个方法。**

## 知识点(大纲对照)

| 要点 | 处理 |
|---|---|
| Java 地位/特点/James Gosling | **了解即可**,不实现 |
| JDK 安装与环境变量 | **动手** |
| 编写→编译→运行 三步 | **动手**(核心) |
| `javap` 反编译 | **动手**(看一眼字节码即可) |
| 编程风格 Allman/K&R、注释 | **了解**,本项目统一用 K&R |

## 要写的代码

**文件**:`Main.java`(默认包,先不分包)

```
类 Main
└── public static void main(String[] args)
```

| 方法 | 签名 | 职责 | 关键逻辑 |
|---|---|---|---|
| 入口 | `public static void main(String[] args)` | 打印就绪信息 | 一行 `System.out.println("Mini-OJ judge ready");` |

> 就这么简单。重点不在代码,在于走通工具链。

## 动手步骤

```bash
# 1. 装 JDK(二选一)
sudo apt install openjdk-21-jdk
java -version && javac -version

# 2. 建目录、写代码
mkdir -p ~/mini-oj && cd ~/mini-oj
nvim Main.java          # 写上面的 Main 类

# 3. 编译 → 运行
javac Main.java         # 生成 Main.class
java Main               # 输出: Mini-OJ judge ready

# 4. 反编译看字节码(理解 .class)
javap -c Main
```

## 验收标准

- [ ] `java -version` 正常,`echo $JAVA_HOME` 有值(如配了)。
- [ ] 终端打印 `Mini-OJ judge ready`。
- [ ] 能用自己的话说清:`Main.java` --javac--> `Main.class` --java(JVM)--> 运行。
- [ ] `javap -c Main` 能看到 `main` 方法的字节码(认出 `getstatic`/`invokevirtual` 即可,不求全懂)。

## 本章产物

一个能编译运行的 `Main.java`。下一章在它里面加判题逻辑。
