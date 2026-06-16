# 07 · 第11章 JDBC + MySQL(里程碑 M5b)

> 目标:把用户、题目元数据、提交记录搬进 MySQL;用 `PreparedStatement` 做 CRUD;用**事务**保证"插入提交 + 更新通过数"要么都成、要么都不成。

## 知识点(大纲对照)

| 要点 | 处理 |
|---|---|
| MySQL 启动 / 客户端、建库建表 | **动手** |
| JDBC 连接、`DriverManager` | **动手** |
| 查询(顺序/条件/排序)、`ResultSet` | **动手** |
| 增删改、`PreparedStatement`、通配符 | **动手**(防注入) |
| 事务:`setAutoCommit/commit/rollback` | **动手**(核心) |
| SQLServer / Derby / Access | **了解即可**,不实现 |

## 0. 准备(命令)

```bash
sudo apt install mysql-server
sudo systemctl start mysql
sudo mysql                       # 进客户端建库(见下)
# 下载驱动放到 lib/:mysql-connector-j-x.x.x.jar
# 编译运行时挂上 classpath:
java -cp "build:lib/*" oj.Main
```
建库建表:
```sql
CREATE DATABASE mini_oj DEFAULT CHARSET utf8mb4;
USE mini_oj;
CREATE TABLE users    (name VARCHAR(32) PRIMARY KEY, solved INT DEFAULT 0);
CREATE TABLE problems (id INT PRIMARY KEY, title VARCHAR(128));
CREATE TABLE submissions (
  id         INT PRIMARY KEY AUTO_INCREMENT,
  problem_id INT, user_name VARCHAR(32),
  status     VARCHAR(8), passed INT, total INT, elapsed_ms BIGINT,
  created_at DATETIME
);
```

## 1. 连接工具 `oj/db/Db.java`

```java
package oj.db;
import java.sql.*;
public class Db {
    private static final String URL  = "jdbc:mysql://localhost:3306/mini_oj?serverTimezone=UTC";
    private static final String USER = "root";
    private static final String PWD  = "你的密码";
    public static Connection get() throws SQLException {
        return DriverManager.getConnection(URL, USER, PWD);   // 驱动会自动注册(JDBC4+)
    }
}
```

## 2. 题目 DAO `oj/db/ProblemDao.java`(PreparedStatement + 查询)

```java
package oj.db;
import java.sql.*;
import java.util.*;
public class ProblemDao {
    public void insert(int id, String title) throws SQLException {
        String sql = "INSERT INTO problems(id, title) VALUES(?, ?)";   // ? 占位符
        try (Connection c = Db.get();
             PreparedStatement ps = c.prepareStatement(sql)) {
            ps.setInt(1, id);
            ps.setString(2, title);
            ps.executeUpdate();
        }
    }
    public List<String> titles() throws SQLException {
        String sql = "SELECT id, title FROM problems ORDER BY id";      // 排序查询
        List<String> out = new ArrayList<>();
        try (Connection c = Db.get();
             PreparedStatement ps = c.prepareStatement(sql);
             ResultSet rs = ps.executeQuery()) {
            while (rs.next())                                            // 遍历游标
                out.add(rs.getInt("id") + ". " + rs.getString("title"));
        }
        return out;
    }
}
```

## 3. 提交 DAO `oj/db/SubmissionDao.java`(事务,本章核心)

```java
package oj.db;
import oj.core.*;
import java.sql.*;
public class SubmissionDao {
    /** 一次提交落库:插入 submission + (若AC)给 user 通过数+1,放在同一个事务里。 */
    public void save(Submission s) throws SQLException {
        String insert = "INSERT INTO submissions"
            + "(problem_id,user_name,status,passed,total,elapsed_ms,created_at)"
            + " VALUES(?,?,?,?,?,?,?)";
        String addSolved = "UPDATE users SET solved = solved + 1 WHERE name = ?";
        Connection c = null;
        try {
            c = Db.get();
            c.setAutoCommit(false);                       // ① 开启事务
            JudgeResult r = s.getResult();
            try (PreparedStatement p1 = c.prepareStatement(insert)) {
                p1.setInt(1, s.getProblemId());
                p1.setString(2, s.getUserName());
                p1.setString(3, r.getStatus().name());    // enum → String
                p1.setInt(4, r.getPassed());
                p1.setInt(5, r.getTotal());
                p1.setLong(6, r.getElapsedMs());
                p1.setObject(7, s.getTime());             // LocalDateTime → DATETIME
                p1.executeUpdate();
            }
            if (r.isAccepted()) {
                try (PreparedStatement p2 = c.prepareStatement(addSolved)) {
                    p2.setString(1, s.getUserName());
                    p2.executeUpdate();
                }
            }
            c.commit();                                   // ② 一起提交
        } catch (SQLException e) {
            if (c != null) c.rollback();                  // ③ 出错回滚,绝不脏写
            throw e;
        } finally {
            if (c != null) { c.setAutoCommit(true); c.close(); }
        }
    }
}
```

## 4. 与文件层衔接

- 题面/用例仍放 `problems/`(`ProblemLoader` 不变),**元数据 + 提交**进 DB。
- 流程:`ProblemLoader.load` 取题 → 判题 → `SubmissionDao.save` 入库(替代/并存 M5a 的 `SubmissionStore`)。

## 验收标准

- [ ] `ProblemDao.insert/titles`、`SubmissionDao.save` 能正常读写 MySQL。
- [ ] 全部 SQL 用 `PreparedStatement` + `?`(没有字符串拼 SQL)。
- [ ] **事务验证**:在 `p2.executeUpdate()` 前手动 `throw new SQLException("test")`,确认 submissions 表也没插进去(回滚生效);去掉后两条一起成功。
- [ ] 能说清"为什么 `PreparedStatement` 比拼字符串安全"(防 SQL 注入)。

## 本章产物 → 下一章

后端(判题 + 文件 + 数据库)齐了。第9章(M5c)给它套一个 **Swing 桌面客户端**:选题、写输入、点提交、看结果表。
