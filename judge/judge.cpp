// =============================================================================
// Mini-OJ 第三代判题机 (judge) —— 单文件、编译型、健壮版
//
//   编译: g++ -O2 -std=c++17 -o judge judge.cpp
//   用法: ./judge --problem <题目目录> --src <提交源码> --lang <cpp|c|python|java>
//                 [--time-ms 1000] [--mem-mb 256] [--special] [--help]
//         (--time / --mem 为 --time-ms / --mem-mb 的别名)
//
//   题目目录: 1.in/1.out, 2.in/2.out, ...(成对、按数字排序;非 .in/.out 文件忽略)
//   输出(stdout 一行 JSON):
//     {"status":"AC","passed":3,"total":3,"time_ms":5,"mem_kb":1840,"detail":""}
//   状态: AC 通过 / WA 答案错 / PE 格式错(仅空白差异) / TLE 超时 / MLE 超内存
//         RE 运行错 / CE 编译错 / ERR 判题机内部错
//
//   能力:
//     - 多语言: C++ / C / Python3 / Java(自动编译/语法检查/构造运行命令)
//     - 资源限制: setrlimit(CPU/AS/FSIZE/STACK) + 父进程墙钟看门狗 + 进程组整组 SIGKILL
//     - 编译也有墙钟上限,防 g++ 模板炸弹挂死
//     - 比对: 逐行去尾空白 + 去末尾空行;--special 开浮点容差(1e-6);
//             仅空白差异判 PE(presentation error)
//     - 健壮: 缺文件/无用例/编译器缺失/异常输出 都有明确状态,不崩
//
//   安全说明: 教学/单机判题机,用 setrlimit + 进程组隔离,但**未做完整沙箱**
//   (无 seccomp/namespace/chroot,不禁系统调用)。只在你信任的本地/实验室环境运行提交。
// =============================================================================
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cmath>
#include <climits>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <time.h>

namespace fs = std::filesystem;

// ─── 通用工具 ───────────────────────────────────────────────────────────────
static std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    std::stringstream ss; ss << f.rdbuf();
    return ss.str();
}

static bool write_bytes(const std::string& path, const std::string& data) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(data.data(), (std::streamsize)data.size());
    return (bool)f;
}

// 归一化: 去掉 \r,逐行去行尾空白,去掉末尾空行
static std::string normalize(const std::string& s) {
    std::vector<std::string> lines;
    std::string cur;
    for (char c : s) {
        if (c == '\n') { lines.push_back(cur); cur.clear(); }
        else if (c != '\r') cur += c;
    }
    lines.push_back(cur);
    for (auto& ln : lines) {
        size_t e = ln.find_last_not_of(" \t\f\v");
        if (e == std::string::npos) ln.clear(); else ln.erase(e + 1);
    }
    while (!lines.empty() && lines.back().empty()) lines.pop_back();
    std::string out;
    for (size_t i = 0; i < lines.size(); ++i) { if (i) out += '\n'; out += lines[i]; }
    return out;
}

// 把所有空白压成单空格、首尾去空白(用于 PE 判定)
static std::string squeeze(const std::string& s) {
    std::string out; bool sp = true;
    for (char c : s) {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v') {
            if (!sp) { out += ' '; sp = true; }
        } else { out += c; sp = false; }
    }
    while (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

static std::string json_escape(const std::string& s) {
    std::string o;
    for (char c : s) {
        switch (c) {
            case '"':  o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n";  break;
            case '\r': o += "\\r";  break;
            case '\t': o += "\\t";  break;
            default:
                if ((unsigned char)c < 0x20) { char b[8]; snprintf(b, sizeof b, "\\u%04x", c); o += b; }
                else o += c;
        }
    }
    return o;
}

static void emit(const std::string& status, int passed, int total,
                 long time_ms, long mem_kb, const std::string& detail) {
    std::string d = detail;
    if (d.size() > 2000) d = d.substr(0, 2000) + " ...(截断)";
    printf("{\"status\":\"%s\",\"passed\":%d,\"total\":%d,"
           "\"time_ms\":%ld,\"mem_kb\":%ld,\"detail\":\"%s\"}\n",
           status.c_str(), passed, total, time_ms, mem_kb, json_escape(d).c_str());
    fflush(stdout);
}

static long now_ms() {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000L + t.tv_nsec / 1000000L;
}

// ─── 编译 / 语法检查(带墙钟上限,捕获 stderr) ──────────────────────────────
// 返回: 0 成功, >0 编译器退出码(CE), -1 启动失败, -2 编译超时
static int run_compiler(const std::vector<std::string>& argv, int wall_ms, std::string& errOut) {
    int pipefd[2];
    if (pipe(pipefd) != 0) { errOut = "pipe 失败"; return -1; }
    pid_t pid = fork();
    if (pid < 0) { errOut = "fork 失败"; return -1; }
    if (pid == 0) {                                   // 子进程: 跑编译器
        setpgid(0, 0);
        close(pipefd[0]);
        dup2(pipefd[1], 2);                           // stderr → 管道
        int devnull = open("/dev/null", O_RDWR);
        if (devnull >= 0) { dup2(devnull, 0); dup2(devnull, 1); }
        std::vector<char*> c;
        for (auto& s : argv) c.push_back(const_cast<char*>(s.c_str()));
        c.push_back(nullptr);
        execvp(c[0], c.data());
        fprintf(stderr, "无法启动编译器: %s", argv[0].c_str());
        _exit(127);
    }
    close(pipefd[1]);
    fcntl(pipefd[0], F_SETFL, O_NONBLOCK);            // 非阻塞读 stderr
    long start = now_ms(); int status = 0; bool done = false; bool killed = false;
    std::string buf; char tmp[4096];
    while (true) {
        ssize_t n = read(pipefd[0], tmp, sizeof tmp);
        if (n > 0) buf.append(tmp, n);
        pid_t w = waitpid(pid, &status, WNOHANG);
        if (w == pid) done = true;
        else if (now_ms() - start > wall_ms) { kill(-pid, SIGKILL); waitpid(pid, &status, 0); killed = true; break; }
        if (done && n <= 0) break;
        if (n < 0) usleep(3000);
    }
    while (true) { ssize_t n = read(pipefd[0], tmp, sizeof tmp); if (n > 0) buf.append(tmp, n); else break; }
    close(pipefd[0]);
    errOut = buf;
    if (killed) return -2;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return -1;
}

// ─── 单测试点运行 ───────────────────────────────────────────────────────────
struct RunResult {
    int  kind = 0;     // 0 正常 / 1 TLE / 2 RE / 3 MLE / 4 内部错
    long cpu_ms = 0;
    long mem_kb = 0;
    int  sig = 0;
    int  exitcode = 0;
};

static RunResult run_one(const std::vector<std::string>& argv,
                         const std::string& infile, const std::string& outfile,
                         int time_ms, int mem_mb, const std::string& lang) {
    RunResult r;
    pid_t pid = fork();
    if (pid < 0) { r.kind = 4; return r; }

    if (pid == 0) {                                   // ── 子进程 ──
        setpgid(0, 0);                                // 独立进程组,便于整组掐断
        int fin  = open(infile.c_str(),  O_RDONLY);
        int fout = open(outfile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        int ferr = open("/dev/null",     O_WRONLY);
        if (fin < 0 || fout < 0) _exit(126);
        dup2(fin, 0); dup2(fout, 1); if (ferr >= 0) dup2(ferr, 2);

        struct rlimit rl;
        rl.rlim_cur = rl.rlim_max = (rlim_t)((time_ms + 999) / 1000) + 1;   // CPU 秒 + 1 宽限
        setrlimit(RLIMIT_CPU, &rl);
        rl.rlim_cur = rl.rlim_max = (rlim_t)64 * 1024 * 1024;               // 输出 ≤64MB
        setrlimit(RLIMIT_FSIZE, &rl);
        rl.rlim_cur = rl.rlim_max = RLIM_INFINITY;                          // 栈不限
        setrlimit(RLIMIT_STACK, &rl);
        rl.rlim_cur = rl.rlim_max = 0;                                      // 不产生 core
        setrlimit(RLIMIT_CORE, &rl);
        // 地址空间硬上限只对 c/cpp 设(java/JVM、python 预留巨量虚拟内存会被误杀);
        // MLE 一律用 ru_maxrss 判定。
        if (mem_mb > 0 && (lang == "cpp" || lang == "c")) {
            rl.rlim_cur = rl.rlim_max = (rlim_t)(mem_mb * 2 + 64) * 1024 * 1024;
            setrlimit(RLIMIT_AS, &rl);
        }

        std::vector<char*> c;
        for (auto& s : argv) c.push_back(const_cast<char*>(s.c_str()));
        c.push_back(nullptr);
        execvp(c[0], c.data());
        _exit(127);                                   // exec 失败
    }

    // ── 父进程: 轮询 + 墙钟看门狗(java 给更宽松启动余量) ──
    long wall_limit = (long)time_ms * 2 + (lang == "java" ? 4000 : 1500);
    long start = now_ms();
    int status = 0; struct rusage ru; bool killed = false;
    for (;;) {
        pid_t w = wait4(pid, &status, WNOHANG, &ru);
        if (w == pid) break;
        if (w < 0) { r.kind = 4; return r; }
        if (now_ms() - start > wall_limit) {
            kill(-pid, SIGKILL);                      // 杀整组(连同子孙进程)
            killed = true;
            wait4(pid, &status, 0, &ru);
            break;
        }
        usleep(2000);
    }

    r.cpu_ms = ru.ru_utime.tv_sec * 1000 + ru.ru_utime.tv_usec / 1000
             + ru.ru_stime.tv_sec * 1000 + ru.ru_stime.tv_usec / 1000;
    r.mem_kb = ru.ru_maxrss;                          // Linux 单位 KB

    bool over_mem = (mem_mb > 0 && r.mem_kb > (long)mem_mb * 1024);
    bool over_cpu = (r.cpu_ms > time_ms);
    if (killed) { r.kind = over_mem ? 3 : 1; return r; }
    if (WIFSIGNALED(status)) {
        r.sig = WTERMSIG(status);
        if (r.sig == SIGXCPU || over_cpu) { r.kind = 1; return r; }   // CPU 超时(含硬限触发的 SIGKILL)
        if (over_mem) { r.kind = 3; return r; }
        r.kind = 2; return r;                          // SIGSEGV/SIGABRT 等 → RE
    }
    r.exitcode = WEXITSTATUS(status);
    if (r.exitcode != 0) { r.kind = over_mem ? 3 : 2; return r; }
    if (over_cpu) { r.kind = 1; return r; }
    if (over_mem) { r.kind = 3; return r; }
    return r;                                          // kind = 0
}

// ─── 输出比对: "AC" / "WA" / "PE" ────────────────────────────────────────────
static std::string compare_output(const std::string& expected, const std::string& actual,
                                  bool special, double eps) {
    std::string e = normalize(expected), a = normalize(actual);
    if (e == a) return "AC";

    if (special) {                                    // 浮点容差: 逐 token 比
        std::istringstream es(e), as(a);
        std::vector<std::string> et, at; std::string w;
        while (es >> w) et.push_back(w);
        while (as >> w) at.push_back(w);
        if (et.size() == at.size()) {
            bool ok = true;
            for (size_t i = 0; i < et.size() && ok; ++i) {
                try {
                    size_t pe, pa;
                    double de = std::stod(et[i], &pe), da = std::stod(at[i], &pa);
                    if (pe != et[i].size() || pa != at[i].size()) ok = (et[i] == at[i]);
                    else {
                        double diff = std::fabs(de - da);
                        double rel  = diff / std::max(1.0, std::fabs(de));
                        ok = (diff <= eps || rel <= eps);
                    }
                } catch (...) { ok = (et[i] == at[i]); }
            }
            if (ok) return "AC";
        }
    }

    if (squeeze(e) == squeeze(a)) return "PE";        // 仅空白排布不同 → PE
    return "WA";
}

// ─── 收集测试点(按数字排序) ───────────────────────────────────────────────
static std::vector<std::pair<std::string,std::string>> collect_cases(const std::string& dir) {
    std::vector<std::pair<long,std::string>> ins;
    std::error_code ec;
    if (fs::is_directory(dir, ec)) {
        for (auto& e : fs::directory_iterator(dir, ec)) {
            if (!e.is_regular_file()) continue;
            fs::path p = e.path();
            if (p.extension() != ".in") continue;
            fs::path out = p; out.replace_extension(".out");
            if (!fs::exists(out)) continue;
            long n = LONG_MAX;
            try { n = std::stol(p.stem().string()); } catch (...) {}
            ins.push_back({n, p.string()});
        }
    }
    std::sort(ins.begin(), ins.end());
    std::vector<std::pair<std::string,std::string>> cases;
    for (auto& kv : ins) {
        fs::path in = kv.second, out = in; out.replace_extension(".out");
        cases.push_back({in.string(), out.string()});
    }
    return cases;
}

// ─── main ────────────────────────────────────────────────────────────────────
static const char* USAGE =
    "用法: judge --problem <dir> --src <file> --lang <cpp|c|python|java>\n"
    "            [--time-ms N] [--mem-mb N] [--special]\n";

int main(int argc, char** argv) {
    std::string problem, src, lang = "cpp";
    int time_ms = 1000, mem_mb = 256;
    bool special = false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> std::string { return (i + 1 < argc) ? std::string(argv[++i]) : std::string(); };
        if      (a == "--problem")  problem = next();
        else if (a == "--src")      src     = next();
        else if (a == "--lang")     lang    = next();
        else if (a == "--time-ms" || a == "--time") time_ms = std::atoi(next().c_str());
        else if (a == "--mem-mb"  || a == "--mem")  mem_mb  = std::atoi(next().c_str());
        else if (a == "--special")  special = true;
        else if (a == "-h" || a == "--help") { printf("%s", USAGE); return 0; }
    }
    for (auto& ch : lang) ch = (char)tolower((unsigned char)ch);
    if (lang == "c++") lang = "cpp";
    if (lang == "py")  lang = "python";

    if (problem.empty() || src.empty()) { fprintf(stderr, "%s", USAGE); emit("ERR", 0, 0, 0, 0, "参数缺失"); return 2; }
    if (!fs::exists(src)) { emit("CE", 0, 0, 0, 0, "源文件不存在: " + src); return 0; }

    auto cases = collect_cases(problem);
    int total = (int)cases.size();
    if (total == 0) { emit("ERR", 0, 0, 0, 0, "题目目录无测试点: " + problem); return 0; }

    char tmpl[] = "/tmp/ojjudge.XXXXXX";
    if (!mkdtemp(tmpl)) { emit("ERR", 0, total, 0, 0, "无法创建临时目录"); return 0; }
    std::string tmp = tmpl;

    // ── 编译 / 准备运行命令 ──
    std::vector<std::string> runcmd;
    std::string cerr;
    if (lang == "cpp" || lang == "c") {
        std::string bin  = tmp + "/a.out";
        std::string cc   = (lang == "cpp") ? "g++" : "gcc";
        std::string std_ = (lang == "cpp") ? "-std=c++17" : "-std=c11";
        int rc = run_compiler({cc, "-O2", std_, "-w", "-o", bin, src}, 10000, cerr);
        if (rc == -2) { emit("CE", 0, total, 0, 0, "编译超时(>10s)"); fs::remove_all(tmp); return 0; }
        if (rc == -1) { emit("CE", 0, total, 0, 0, "无法启动编译器 " + cc + " — 是否已安装?"); fs::remove_all(tmp); return 0; }
        if (rc != 0)  { emit("CE", 0, total, 0, 0, cerr.empty() ? "编译失败" : cerr); fs::remove_all(tmp); return 0; }
        runcmd = { bin };
    } else if (lang == "java") {
        std::string mainJava = tmp + "/Main.java";    // public class 名须与文件名一致
        if (!write_bytes(mainJava, read_file(src))) { emit("ERR", 0, total, 0, 0, "写 Main.java 失败"); fs::remove_all(tmp); return 0; }
        int rc = run_compiler({"javac", "-d", tmp, mainJava}, 15000, cerr);
        if (rc == -2) { emit("CE", 0, total, 0, 0, "编译超时(>15s)"); fs::remove_all(tmp); return 0; }
        if (rc == -1) { emit("CE", 0, total, 0, 0, "无法启动 javac — JDK 是否已安装?"); fs::remove_all(tmp); return 0; }
        if (rc != 0)  { emit("CE", 0, total, 0, 0, cerr.empty() ? "编译失败(public 类名须为 Main)" : cerr); fs::remove_all(tmp); return 0; }
        runcmd = { "java", "-XX:+UseSerialGC", "-cp", tmp, "Main" };
    } else if (lang == "python") {
        int rc = run_compiler({"python3", "-m", "py_compile", src}, 10000, cerr);
        if (rc == -1) { emit("CE", 0, total, 0, 0, "无法启动 python3 — 是否已安装?"); fs::remove_all(tmp); return 0; }
        if (rc != 0)  { emit("CE", 0, total, 0, 0, cerr.empty() ? "语法错误" : cerr); fs::remove_all(tmp); return 0; }
        runcmd = { "python3", src };
    } else {
        emit("ERR", 0, total, 0, 0, "未知语言: " + lang); fs::remove_all(tmp); return 0;
    }

    // ── 逐测试点(首个非 AC 即停) ──
    int passed = 0; long maxTime = 0, maxMem = 0;
    std::string status = "AC", detail = "";
    for (int i = 0; i < total; ++i) {
        std::string outf = tmp + "/out.txt";
        RunResult r = run_one(runcmd, cases[i].first, outf, time_ms, mem_mb, lang);
        maxTime = std::max(maxTime, r.cpu_ms);
        maxMem  = std::max(maxMem,  r.mem_kb);
        std::string tag = "case#" + std::to_string(i + 1);
        if (r.kind == 1) { status = "TLE"; detail = tag + " 超时"; break; }
        if (r.kind == 3) { status = "MLE"; detail = tag + " 超内存"; break; }
        if (r.kind == 2) { status = "RE";  detail = tag + " 运行错误(signal " + std::to_string(r.sig)
                                                  + ", exit " + std::to_string(r.exitcode) + ")"; break; }
        if (r.kind == 4) { status = "ERR"; detail = tag + " 内部错误"; break; }

        std::string verdict = compare_output(read_file(cases[i].second), read_file(outf), special, 1e-6);
        if (verdict == "AC") { passed++; }
        else { status = verdict; detail = tag + (verdict == "PE" ? " 格式错(仅空白差异)" : " 输出不匹配"); break; }
    }

    emit(status, passed, total, maxTime, maxMem, detail);
    fs::remove_all(tmp);
    return 0;
}
