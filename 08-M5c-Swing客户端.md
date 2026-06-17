# 08 · M5c Swing 桌面客户端（Ch9，大前端）

> 判题逻辑已经具备：数据库存题、C++ 判题机真正跑代码。现在缺少的是一扇窗户——用户还在用命令行粘贴代码、手动查库。本章用 Java Swing 为 Mini-OJ 装上可视化界面，把前三代的成果串联成一个完整的桌面应用。

## 【核心痛点】

M5b 之后，系统后端已经相当完善：ProblemService 从数据库读取题目元数据，ProblemLoader 加载测试点，MachineJudge 调用外部 C++ 判题机真实编译运行。但所有交互都停留在命令行或单元测试层面。

具体痛点如下：

1. **无题目浏览**：用户不知道有哪些题，必须手动查数据库或翻代码。
2. **无语言选择**：提交时语言类型硬编码在测试脚本里。
3. **无源码编辑区**：用户只能把源文件路径写死，无法在界面里直接粘贴或编写代码。
4. **无结果展示**：判题完成后结果只打印在控制台，没有弹窗或表格呈现给用户。

本章目标：用 Swing 构建一个 MVC 结构的桌面客户端，让用户在图形界面里选题、选语言、贴代码、提交，然后看到 AC/WA/TLE 等结果。

---

## 【引入课本知识点】

本章对应 Ch9——Java 图形用户界面（Swing）。

**JFrame 与 BorderLayout**

`JFrame` 是 Swing 的顶层窗口容器。`BorderLayout` 是其默认布局管理器，将窗口分为 NORTH/SOUTH/CENTER/EAST/WEST 五个区域，适合"工具栏在上、内容在中、操作按钮在下"的经典布局。

```java
JFrame frame = new JFrame("Mini-OJ");
frame.setLayout(new BorderLayout());
frame.add(toolbar, BorderLayout.NORTH);
frame.add(scrollPane, BorderLayout.CENTER);
frame.add(buttonPanel, BorderLayout.SOUTH);
```

**JComboBox — 下拉选择框**

用于语言选择（cpp / python）和题目选择。泛型版本 `JComboBox<String>` 类型安全，通过 `getSelectedItem()` 获取当前选中值。题目下拉框统一用 `JComboBox<String>` 显示 "id 标题"，**不要把它当成 `JComboBox<Integer>`**，取题号要靠 OjFrame 内部维护的 id 列表换算。

```java
JComboBox<String> langBox = new JComboBox<>(new String[]{"cpp", "python"});
String lang = (String) langBox.getSelectedItem();
```

**JTextArea — 多行文本编辑区**

源码编辑区的核心组件。需要用 `JScrollPane` 包裹以支持滚动，设置等宽字体提升代码可读性。

```java
JTextArea codeArea = new JTextArea(20, 60);
codeArea.setFont(new Font("Monospaced", Font.PLAIN, 13));
JScrollPane scroll = new JScrollPane(codeArea);
```

**JButton + Lambda 事件监听**

Ch9 的重点之一：用函数式接口 `ActionListener`（单方法接口）配合 Lambda 表达式绑定按钮事件，替代匿名内部类写法，代码更简洁。

```java
JButton submitBtn = new JButton("提交");
submitBtn.addActionListener(e -> controller.onSubmit());
```

`ActionListener` 是函数式接口（`@FunctionalInterface`），`actionPerformed(ActionEvent e)` 是其唯一抽象方法，Lambda `e -> ...` 正是该方法的实现体。

**JLabel — 状态标签**

工具栏里放一个 `JLabel`，用来实时显示当前状态（"就绪"、判题结果文本等），比每次都弹窗更轻量。本章 OjFrame 会真实持有这个状态标签字段并对外暴露 `getResultLabel()`，下一章（M6a）会直接复用它做"评测中…"提示。

**JOptionPane — 状态弹窗**

用于在判题完成后弹出结果对话框，无需手动创建 `JDialog`。

```java
JOptionPane.showMessageDialog(frame, result.toString(), "判题结果",
    JOptionPane.INFORMATION_MESSAGE);
```

**JTable — 提交历史表格**

通过 `DefaultTableModel` 动态添加行，展示历史提交的题号、语言、状态、耗时。

**EDT（Event Dispatch Thread）规则**

Swing 不是线程安全的，所有 UI 操作必须在 EDT 上执行。长时间操作（如调用 MachineJudge）必须在后台线程（`SwingWorker` 或普通线程）里执行，完成后用 `SwingUtilities.invokeLater` 回到 EDT 更新界面。

---

## 【三代演进定位】

本章**不产生新判题机代**。Swing 客户端是"大前端"，其定位是：

```
用户 → OjFrame（View）→ OjController（Controller）
         ↓                      ↓
    ProblemService（Model）   MachineJudge（第三代）
         ↓                      ↓
      数据库（M5b）          C++ 判题机进程
         ↓                      ↓
    SubmissionDao.save()     JudgeResult 返回
```

- **第一代**（M1–M3，已完成）：Solution.solve 在 JVM 内执行，题目硬编码，无 UI。
- **第二代**（M4）：反射工厂 + 单文件配置，解耦题目元数据，仍无 UI。
- **第三代**（M5a）：ProcessBuilder 调用外部 C++ 判题机，真实编译运行，仍无 UI。
- **M5c（本章）**：Swing 客户端直接调用第三代 MachineJudge 和 M5b 的 DAO，不经过网络（**不使用 Socket/ServerSocket**），所有调用均为本地方法调用。

删除说明：Ch13 网络内容已从课程中移除，因此客户端与判题机之间**不存在任何 Socket 通信**。OjController 直接持有 MachineJudge 和 SubmissionDao 的引用，在同一 JVM 进程内完成全部工作。

---

## 【新产物架构】

### 涉及的类与接口

| 类/接口 | 包 | 职责 |
|---|---|---|
| `OjFrame` | `oj.gui` | View 层；持有全部 Swing 组件；不含业务逻辑 |
| `OjController` | `oj.gui` | Controller 层；处理提交事件；协调 Model |
| `ProblemService` | `oj.service` | Model 层；从 DB + FS 加载题目（M5b 已实现） |
| `MachineJudge` | `oj.judge` | 第三代判题机；ProcessBuilder 调用外部二进制 |
| `SubmissionDao` | `oj.db` | 持久化提交记录到数据库（M5b 已实现） |
| `Submission` | `oj.core` | 提交数据模型（M5a 已实现） |
| `JudgeResult` | `oj.core` | 判题结果数据模型 |

---

### OjFrame（View 层）

OjFrame 负责所有 Swing 组件的创建和布局，对外暴露 getter 让 Controller 读取输入、写入结果，不直接调用任何业务逻辑。题目下拉用 `JComboBox<String>`，内部用一个 `problemIds` 列表把下标映射回真正的题号，`getSelectedProblemId()` 在没有选中时返回 `-1`。工具栏里有一个真实的状态标签 `resultLabel`，并通过 `getResultLabel()` 暴露给 Controller（下一章 M6a 会复用它）。

```java
package oj.gui;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.util.List;

public class OjFrame extends JFrame {

    private final JComboBox<String> problemBox;
    private final JComboBox<String> langBox;
    private final JTextArea codeArea;
    private final JButton submitBtn;
    private final JLabel resultLabel;
    private final DefaultTableModel historyModel;
    private final JTable historyTable;

    private List<Integer> problemIds;   // 与 problemBox 下标对应

    public OjFrame() {
        super("Mini-OJ 桌面客户端");
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setSize(900, 650);
        setLayout(new BorderLayout(6, 6));

        // ── 北部工具栏 ──────────────────────────────────────────
        JPanel toolbar = new JPanel(new FlowLayout(FlowLayout.LEFT, 8, 4));

        toolbar.add(new JLabel("题目："));
        problemBox = new JComboBox<>();
        problemBox.setPreferredSize(new Dimension(260, 26));
        toolbar.add(problemBox);

        toolbar.add(new JLabel("语言："));
        langBox = new JComboBox<>(new String[]{"cpp", "python"});
        toolbar.add(langBox);

        submitBtn = new JButton("提交");
        toolbar.add(submitBtn);

        // 状态标签：实时显示判题状态，供 Controller 写入
        resultLabel = new JLabel("就绪");
        resultLabel.setForeground(new Color(0x33, 0x66, 0x99));
        toolbar.add(resultLabel);

        add(toolbar, BorderLayout.NORTH);

        // ── 中部源码编辑区 ────────────────────────────────────
        codeArea = new JTextArea();
        codeArea.setFont(new Font("Monospaced", Font.PLAIN, 13));
        codeArea.setTabSize(4);
        JScrollPane codeScroll = new JScrollPane(codeArea);
        codeScroll.setBorder(BorderFactory.createTitledBorder("源码"));
        add(codeScroll, BorderLayout.CENTER);

        // ── 南部提交历史表格 ──────────────────────────────────
        String[] cols = {"提交 ID", "题目 ID", "语言", "状态", "通过/总计", "耗时(ms)"};
        historyModel = new DefaultTableModel(cols, 0) {
            @Override
            public boolean isCellEditable(int row, int col) {
                return false;
            }
        };
        historyTable = new JTable(historyModel);
        historyTable.setFillsViewportHeight(true);
        JScrollPane tableScroll = new JScrollPane(historyTable);
        tableScroll.setPreferredSize(new Dimension(0, 180));
        tableScroll.setBorder(BorderFactory.createTitledBorder("提交历史"));
        add(tableScroll, BorderLayout.SOUTH);

        setLocationRelativeTo(null);
    }

    // ── 初始化题目下拉列表（由 Controller 在加载完数据后调用）──
    public void loadProblems(java.util.List<Integer> ids, java.util.List<String> titles) {
        this.problemIds = ids;
        problemBox.removeAllItems();
        for (int i = 0; i < ids.size(); i++) {
            problemBox.addItem(ids.get(i) + " " + titles.get(i));
        }
    }

    // ── 向历史表格追加一行 ────────────────────────────────────
    public void appendHistory(int subId, int probId, String lang,
                              String status, String passTotal, long ms) {
        historyModel.addRow(new Object[]{subId, probId, lang, status, passTotal, ms});
        int last = historyModel.getRowCount() - 1;
        historyTable.scrollRectToVisible(historyTable.getCellRect(last, 0, true));
    }

    // ── Getters 供 Controller 读取 ───────────────────────────
    public int getSelectedProblemId() {
        int idx = problemBox.getSelectedIndex();
        if (idx < 0 || problemIds == null) return -1;
        return problemIds.get(idx);
    }

    public String getSelectedLang() {
        return (String) langBox.getSelectedItem();
    }

    public String getSourceCode() {
        return codeArea.getText();
    }

    public JButton getSubmitBtn() {
        return submitBtn;
    }

    public JLabel getResultLabel() {
        return resultLabel;
    }
}
```

---

### OjController（Controller 层）

OjController 持有 Model 的全部引用，构造签名为 `(OjFrame view, ProblemService svc, MachineJudge judge, SubmissionDao dao)`。构造时用 `svc.listMeta()` 把题目填进下拉框，并绑定提交按钮的 Lambda 事件监听器。提交操作在后台线程执行，避免阻塞 EDT。

注意几个与全局契约对齐的关键点：

- ProblemService 用 `listMeta()` / `meta(int)`，不是 `listAll()`。
- `SubmissionDao.save(Submission)` 返回 `void`，所以拿提交 ID 要先 `dao.save(sub)` 再 `sub.getId()`（id 由 Submission 静态计数自增）。
- 临时源文件写好后**不要立即删除**——它的路径要存进 Submission 的 `srcPath`，留作记录。

```java
package oj.gui;

import oj.core.JudgeResult;
import oj.core.ProblemMeta;
import oj.core.Submission;
import oj.db.SubmissionDao;
import oj.judge.MachineJudge;
import oj.service.ProblemService;

import javax.swing.*;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class OjController {

    private final OjFrame view;
    private final ProblemService svc;
    private final MachineJudge judge;
    private final SubmissionDao dao;

    private static final String PROBLEMS_DIR = "problems";
    private static final String USER_NAME    = "student";   // 演示用固定用户

    public OjController(OjFrame view,
                        ProblemService svc,
                        MachineJudge judge,
                        SubmissionDao dao) {
        this.view  = view;
        this.svc   = svc;
        this.judge = judge;
        this.dao   = dao;

        initProblems();
        bindEvents();
    }

    // 从数据库加载题目列表，填充下拉框
    private void initProblems() {
        try {
            Map<Integer, ProblemMeta> all = svc.listMeta();
            List<Integer> ids    = new ArrayList<>(all.keySet());
            List<String>  titles = new ArrayList<>();
            ids.forEach(id -> titles.add(all.get(id).getTitle()));
            view.loadProblems(ids, titles);
        } catch (Exception ex) {
            JOptionPane.showMessageDialog(view,
                "加载题目失败：" + ex.getMessage(), "错误",
                JOptionPane.ERROR_MESSAGE);
        }
    }

    // 绑定提交按钮：Lambda → 后台线程执行判题，SwingUtilities 回 EDT 更新 UI
    private void bindEvents() {
        view.getSubmitBtn().addActionListener(e -> onSubmit());
    }

    void onSubmit() {
        int problemId = view.getSelectedProblemId();
        if (problemId < 0) {
            JOptionPane.showMessageDialog(view, "请先选择一道题目。",
                "提示", JOptionPane.WARNING_MESSAGE);
            return;
        }

        String src = view.getSourceCode().trim();
        if (src.isEmpty()) {
            JOptionPane.showMessageDialog(view, "源码不能为空。",
                "提示", JOptionPane.WARNING_MESSAGE);
            return;
        }

        String lang = view.getSelectedLang();
        view.getSubmitBtn().setEnabled(false);
        view.getResultLabel().setText("评测中…");

        // 后台线程：写源文件 → 调 MachineJudge → 入库 → 回 EDT 更新 UI
        new Thread(() -> {
            JudgeResult result = null;
            int subId = -1;
            try {
                // 1. 落源文件（保留路径，存进 Submission.srcPath，不立即删除）
                String ext = lang.equals("cpp") ? ".cpp" : ".py";
                Path   tmp = Files.createTempFile("oj_submit_", ext);
                Files.write(tmp, src.getBytes(StandardCharsets.UTF_8));

                // 2. 获取题目元数据（时间限制）
                ProblemMeta meta = svc.meta(problemId);
                long timeMs = meta.getTimeLimitMs();
                int  memMb  = 256;   // 默认内存限制

                // 3. 调用第三代 MachineJudge（本地进程，无网络）
                String problemDir = PROBLEMS_DIR + "/" + problemId;
                result = judge.judge(problemDir, tmp.toString(),
                    lang, timeMs, memMb);

                // 4. 构造提交记录并持久化到数据库
                //    save() 返回 void，提交 ID 由 Submission 自增，存库后用 getId() 取回
                Submission sub = new Submission(
                    problemId, USER_NAME, lang, tmp.toString());
                sub.setResult(result);
                dao.save(sub);
                subId = sub.getId();

            } catch (IOException | InterruptedException ex) {
                Thread.currentThread().interrupt();
                final String msg = ex.getMessage();
                SwingUtilities.invokeLater(() -> {
                    view.getResultLabel().setText("提交出错");
                    view.getSubmitBtn().setEnabled(true);
                    JOptionPane.showMessageDialog(view,
                        "提交出错：" + msg, "错误",
                        JOptionPane.ERROR_MESSAGE);
                });
                return;
            }

            // 回到 EDT 更新界面
            final JudgeResult finalResult = result;
            final int         finalSubId  = subId;
            SwingUtilities.invokeLater(() -> {
                view.getSubmitBtn().setEnabled(true);
                view.getResultLabel().setText(finalResult.toString());

                // 追加历史表格一行
                view.appendHistory(
                    finalSubId, problemId, lang,
                    finalResult.getStatus().name(),
                    finalResult.getPassed() + "/" + finalResult.getTotal(),
                    finalResult.getElapsedMs());

                // 弹窗显示判题结果
                int msgType = finalResult.isAccepted()
                    ? JOptionPane.INFORMATION_MESSAGE
                    : JOptionPane.WARNING_MESSAGE;
                JOptionPane.showMessageDialog(view,
                    finalResult.toString() + "\n" + finalResult.getDetail(),
                    "判题结果", msgType);
            });
        }).start();
    }
}
```

---

### 程序入口（Main 类）

Main 按全局契约组装 Model 层：`ProblemDao` / `SubmissionDao` 都是**无参构造**（内部用 `Db.get()` 取单例连接，没有 `Db.connect()`），`ProblemService(ProblemDao, ProblemLoader)`、`MachineJudge(String judgeBinary)` 都按契约构造。所有 Swing 操作放进 `SwingUtilities.invokeLater`。

```java
package oj.gui;

import oj.db.ProblemDao;
import oj.db.SubmissionDao;
import oj.io.ProblemLoader;
import oj.judge.MachineJudge;
import oj.service.ProblemService;

import javax.swing.*;

public class Main {

    public static void main(String[] args) {
        // 初始化 Model 层（DAO 无参构造，内部用 Db.get() 单例连接）
        ProblemService svc   = new ProblemService(new ProblemDao(),
                                                  new ProblemLoader("problems"));
        MachineJudge   judge = new MachineJudge("judge/judge");
        SubmissionDao  dao   = new SubmissionDao();

        // 所有 Swing 操作必须在 EDT 上执行
        SwingUtilities.invokeLater(() -> {
            OjFrame f = new OjFrame();
            new OjController(f, svc, judge, dao);
            f.setVisible(true);
        });
    }
}
```

---

### 关键交互时序（文字版）

```
[EDT] 用户点击"提交"
  → OjController.onSubmit()
      → 校验输入（EDT）
      → 禁用按钮 + 状态标签显示"评测中…"（EDT）
      → 启动新线程（非 EDT）
          → Files.createTempFile / Files.write  ← 写源文件（路径留作 srcPath）
          → svc.meta(problemId)                 ← 取题目时限
          → judge.judge(...)                    ← ProcessBuilder，第三代
          → new Submission(...) + setResult     ← 构造提交记录
          → dao.save(sub); subId = sub.getId()  ← JDBC 持久化（save 返回 void）
          → SwingUtilities.invokeLater(...)     ← 回 EDT
              → 状态标签写入结果文本
              → view.appendHistory(...)         ← 更新表格
              → JOptionPane.showMessageDialog   ← 弹窗
              → 恢复按钮
```

**没有任何 Socket/ServerSocket/网络调用**。MachineJudge 通过 `ProcessBuilder` 启动本地可执行文件（系统进程），SubmissionDao 通过 JDBC 访问本地数据库，两者均为进程内的本地调用。

---

## 验收标准

1. **题目下拉**：启动后 `JComboBox<String>` 自动填充数据库中全部题目（"id 标题"），按 id 升序，无需手动输入题号；取题号一律走 `getSelectedProblemId()`，未选中返回 `-1`。
2. **语言选择**：`JComboBox` 包含 `cpp` 和 `python` 两项，选中值传递给 MachineJudge。
3. **源码编辑**：`JTextArea` 使用等宽字体，支持 Tab 缩进，可粘贴任意长度源码。
4. **提交不冻结 UI**：点击提交后按钮立即变灰、状态标签显示"评测中…"，判题期间界面保持可响应；判题完成后按钮恢复。
5. **状态标签**：OjFrame 工具栏里有一个真实的 `JLabel`（`getResultLabel()` 暴露），提交过程中显示"评测中…"，完成后显示 `JudgeResult.toString()`（如 `AC 3/3 (47ms)`）。
6. **结果弹窗**：判题完成后弹出 `JOptionPane`，AC 时显示 `INFORMATION_MESSAGE`，非 AC 时显示 `WARNING_MESSAGE`，内容为 `JudgeResult.toString()` 加 detail 字段。
7. **历史表格**：每次提交后向 `JTable` 追加一行，包含提交 ID、题目 ID、语言、状态、通过/总计、耗时，表格自动滚动到最新行。
8. **持久化**：提交记录通过 `dao.save(sub)`（返回 void）写入数据库，提交 ID 用 `sub.getId()` 取回；源文件路径作为 `srcPath` 一并入库（写文件后不立即删除）。
9. **构造契约**：DAO 无参构造（内部 `Db.get()`），`ProblemService(ProblemDao, ProblemLoader)`、`MachineJudge(String)` 按契约组装；OjController 构造签名为 `(OjFrame, ProblemService, MachineJudge, SubmissionDao)`。
10. **无网络依赖**：全程无 `Socket`、`ServerSocket`、`URL`、`HttpURLConnection` 等任何网络 API；MachineJudge 和 SubmissionDao 均为本地调用。
11. **MVC 分离**：`OjFrame` 不包含任何业务逻辑，`OjController` 不直接操作 Swing 组件样式，`ProblemService`/`MachineJudge`/`SubmissionDao` 不感知 UI 层。
12. **EDT 合规**：所有 `view.*` 调用均在 EDT 上执行；MachineJudge 调用在独立线程；通过 `SwingUtilities.invokeLater` 回到 EDT，无线程安全问题。
