# 08 · 第9章 Swing 桌面客户端(里程碑 M5c)

> 目标:给 Mini-OJ 套一个图形界面——左边选题、中间写输入、点"提交"判题、下方表格看每个用例结果。按 MVC 思路分层。

## 知识点(大纲对照)

| 要点 | 处理 |
|---|---|
| `JFrame`、布局(`BorderLayout` 等) | **动手** |
| 常用组件:`JList`/`JTextArea`/`JButton`/`JTable` | **动手** |
| 菜单条 `JMenuBar`/`JMenu`/`JMenuItem` | **动手** |
| 事件处理:`ActionEvent` + Lambda 监听器 | **动手**(核心) |
| 对话框 `JOptionPane`(消息/确认) | **动手** |
| MVC 结构 | **动手**(分层) |
| `JTree`、Item/Document/Mouse/Key/Focus 事件、颜色对话框、按钮绑键盘 | **了解即可**,挑 1-2 个点到为止 |

## 设计(MVC)

- **Model**:已有的 `ProblemDao`/`ProblemLoader` + `Judge`(后端逻辑,GUI 不重写)。
- **View**:`OjFrame`(纯界面与控件)。
- **Controller**:监听器(本项目简单,直接用 Lambda 写在 `OjFrame` 里;复杂了再拆 `OjController`)。

## 主窗口 `oj/gui/OjFrame.java`(核心)

```java
package oj.gui;
import oj.core.*;
import oj.io.ProblemLoader;
import oj.judge.*;
import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;

public class OjFrame extends JFrame {
    private final JList<String> problemList = new JList<>(new String[]{"1. A+B"});
    private final JTextArea inputArea = new JTextArea("1 2");
    private final DefaultTableModel resultModel =
            new DefaultTableModel(new String[]{"用例", "期望", "状态"}, 0);
    private final JTable resultTable = new JTable(resultModel);

    public OjFrame() {
        super("Mini-OJ");
        setJMenuBar(buildMenu());
        setLayout(new BorderLayout());
        add(new JScrollPane(problemList), BorderLayout.WEST);     // 左:题目列表
        add(new JScrollPane(inputArea),   BorderLayout.CENTER);   // 中:输入编辑
        add(new JScrollPane(resultTable), BorderLayout.SOUTH);    // 下:结果表

        JButton submit = new JButton("提交");
        submit.addActionListener(e -> onSubmit());                // Lambda 监听 ActionEvent
        add(submit, BorderLayout.EAST);

        setSize(720, 520);
        setLocationRelativeTo(null);
        setDefaultCloseOperation(DISPOSE_ON_CLOSE);
    }

    private JMenuBar buildMenu() {
        JMenuBar bar = new JMenuBar();
        JMenu file = new JMenu("文件");
        JMenuItem quit = new JMenuItem("退出");
        quit.addActionListener(e -> dispatchEvent(            // 确认对话框
            new java.awt.event.WindowEvent(this, java.awt.event.WindowEvent.WINDOW_CLOSING)));
        file.add(quit);
        bar.add(file);
        return bar;
    }

    /** 点"提交":取选中题→判题→填结果表→弹消息框 */
    private void onSubmit() {
        try {
            int idx = problemList.getSelectedIndex();
            if (idx < 0) { JOptionPane.showMessageDialog(this, "先选一道题"); return; }
            Problem p = ProblemLoader.load(idx + 1);
            Solution s = new AplusB();                  // 教学版:用内置解法;真实场景由用户提供
            JudgeResult r = new StandardJudge().judge(p, s);
            fillResult(p, r);
            JOptionPane.showMessageDialog(this, r.toString(), "判题结果",
                    r.isAccepted() ? JOptionPane.INFORMATION_MESSAGE : JOptionPane.WARNING_MESSAGE);
        } catch (Exception ex) {
            JOptionPane.showMessageDialog(this, "出错: " + ex.getMessage(),
                    "错误", JOptionPane.ERROR_MESSAGE);   // 错误对话框
        }
    }

    private void fillResult(Problem p, JudgeResult r) {
        resultModel.setRowCount(0);
        TestCase[] cs = p.getCases();
        for (int i = 0; i < cs.length; i++) {
            String st = (i < r.getPassed()) ? "AC" : r.getStatus().name();   // 简化演示
            resultModel.addRow(new Object[]{ "#" + i, cs[i].getExpected().trim(), st });
        }
    }
}
```

## 启动 `oj/gui/Launcher.java`

```java
package oj.gui;
import javax.swing.SwingUtilities;
public class Launcher {
    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> new OjFrame().setVisible(true));  // 在 EDT 上建界面
    }
}
```
```bash
java -cp "build:lib/*" oj.gui.Launcher
```

## 验收标准

- [ ] 窗口能显示:左题目列表、中输入框、下结果表、提交按钮、文件菜单。
- [ ] 选题后点"提交"→结果表刷新 + 弹出结果对话框(AC/WA 图标不同)。
- [ ] 出错(如没选题/题目文件缺失)弹错误对话框,程序不崩。
- [ ] 至少用到:`ActionEvent` 监听、`JOptionPane` 对话框、一个布局管理器、`JTable`。
- [ ] 能说清哪部分是 Model / View / Controller。

## 本章产物 → 下一章

界面好了,但判题是"点一下、卡一下、单个跑"。第12章(M6a)引入**多线程判题队列**:提交入队、后台线程判、`SwingWorker` 不卡界面,并用线程做 **TLE 超时**。
