// =============================================================================
// Mini-OJ 判题机 (judge) —— 单文件、编译型
//
//   编译: g++ -O2 -std=c++17 -o judge judge.cpp
//   用法: ./judge --problem <题目目录> --src <提交源码> --lang <cpp|python>
//                 [--time-ms 1000] [--mem-mb 256]
//
//   题目目录约定: 1.in/1.out, 2.in/2.out, ... (成对出现, 按数字排序)
//   输出(stdout, 一行 JSON, 供调用方解析):
//     {"status":"AC","passed":3,"total":3,"time_ms":5,"mem_kb":1840,"detail":""}
//   状态: AC 通过 / WA 答案错 / TLE 超时 / MLE 超内存 / RE 运行错 / CE 编译错 / ERR 内部错
//
//   说明: 这是教学/单机用判题机, 资源限制用 setrlimit + 墙钟看门狗,
//        未做完整沙箱(seccomp/cgroups), 只在你信任的本地环境运行提交。
// =============================================================================
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
#include <time.h>

namespace fs = std::filesystem;

// ---- 小工具 ----------------------------------------------------------------
static std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    std::stringstream ss; ss << f.rdbuf();
    return ss.str();
}

// 输出归一化: 去掉每行行尾空白 + 去掉末尾空行, 然后逐字符比较
static std::string normalize(const std::string& s) {
    std::vector<std::string> lines;
    std::string cur;
    for (char c : s) {
        if (c == '\n') { lines.push_back(cur); cur.clear(); }
        else if (c == '\r') { /* 吃掉 CR, 兼容 Windows 换行 */ }
        else cur += c;
    }
    lines.push_back(cur);
    for (auto& ln : lines) {
        size_t e = ln.find_last_not_of(" \t");
        if (e == std::string::npos) ln.clear(); else ln.erase(e + 1);
    }
    while (!lines.empty() && lines.back().empty()) lines.pop_back();
    std::string out;
    for (size_t i = 0; i < lines.size(); ++i) { if (i) out += '\n'; out += lines[i]; }
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

// 一行 JSON 结果 + 退出
static void emit(const std::string& status, int passed, int total,
                 long time_ms, long mem_kb, const std::string& detail) {
    printf("{\"status\":\"%s\",\"passed\":%d,\"total\":%d,"
           "\"time_ms\":%ld,\"mem_kb\":%ld,\"detail\":\"%s\"}\n",
           status.c_str(), passed, total, time_ms, mem_kb, json_escape(detail).c_str());
    fflush(stdout);
}

// ---- 编译 / 语法检查 -------------------------------------------------------
// 用 system() 调 g++ / python3, 错误输出重定向到 tmp 下文件后读回。
static bool compile_cpp(const std::string& src, const std::string& bin,
                        const std::string& tmp, std::string& err) {
    std::string ef = tmp + "/compile.err";
    std::string cmd = "g++ -O2 -std=c++17 -w -o \"" + bin + "\" \"" + src + "\" 2> \"" + ef + "\"";
    int rc = system(cmd.c_str());
    err = read_file(ef);
    return rc == 0;
}

static bool pycheck(const std::string& src, const std::string& tmp, std::string& err) {
    std::string ef = tmp + "/compile.err";
    std::string cmd = "python3 -m py_compile \"" + src + "\" 2> \"" + ef + "\"";
    int rc = system(cmd.c_str());
    err = read_file(ef);
    return rc == 0;
}

// ---- 单测试点运行 ----------------------------------------------------------
struct RunResult {
    int  kind = 0;     // 0 正常结束 / 1 TLE / 2 RE / 3 MLE / 4 内部错
    long time_ms = 0;  // CPU 时间(用户态+内核态)
    long mem_kb  = 0;  // 峰值内存 ru_maxrss
    int  sig = 0;      // 被信号杀死时的信号号
};

static RunResult run_one(const std::vector<std::string>& argv,
                         const std::string& infile, const std::string& outfile,
                         int time_ms, int mem_mb) {
    RunResult r;
    pid_t pid = fork();
    if (pid < 0) { r.kind = 4; return r; }

    if (pid == 0) {                                   // ---- 子进程 ----
        int fin  = open(infile.c_str(),  O_RDONLY);
        int fout = open(outfile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        int ferr = open("/dev/null",     O_WRONLY);
        if (fin < 0 || fout < 0) _exit(127);
        dup2(fin, 0); dup2(fout, 1); if (ferr >= 0) dup2(ferr, 2);

        struct rlimit rl;
        // CPU 时间(秒): 向上取整 + 1 秒宽限, 超出内核发 SIGXCPU -> 判 TLE
        rl.rlim_cur = rl.rlim_max = (rlim_t)(time_ms / 1000) + 1;
        setrlimit(RLIMIT_CPU, &rl);
        // 地址空间安全上限(防把机器内存吃光): 给到限额 2 倍 + 64MB 余量;
        // 真正的 MLE 判定靠下面的 ru_maxrss, 避免正常程序因虚拟内存预留误杀。
        if (mem_mb > 0) {
            rlim_t cap = (rlim_t)(mem_mb * 2 + 64) * 1024 * 1024;
            rl.rlim_cur = rl.rlim_max = cap;
            setrlimit(RLIMIT_AS, &rl);
        }
        // 输出大小上限 64MB, 防输出洪水
        rl.rlim_cur = rl.rlim_max = (rlim_t)64 * 1024 * 1024;
        setrlimit(RLIMIT_FSIZE, &rl);

        std::vector<char*> c;
        for (auto& s : argv) c.push_back(const_cast<char*>(s.c_str()));
        c.push_back(nullptr);
        execvp(c[0], c.data());
        _exit(127);                                   // exec 失败
    }

    // ---- 父进程: 轮询等待 + 墙钟看门狗 ----
    long wall_limit = (long)time_ms * 2 + 1000;       // 墙钟上限(含启动开销, 抓 sleep/IO 型超时)
    struct timespec t0; clock_gettime(CLOCK_MONOTONIC, &t0);
    int status = 0; struct rusage ru; bool killed = false;
    for (;;) {
        pid_t w = wait4(pid, &status, WNOHANG, &ru);
        if (w == pid) break;
        if (w < 0) { r.kind = 4; return r; }
        struct timespec t1; clock_gettime(CLOCK_MONOTONIC, &t1);
        long ms = (t1.tv_sec - t0.tv_sec) * 1000 + (t1.tv_nsec - t0.tv_nsec) / 1000000;
        if (ms > wall_limit) { kill(pid, SIGKILL); killed = true; wait4(pid, &status, 0, &ru); break; }
        usleep(2000);
    }

    r.time_ms = ru.ru_utime.tv_sec * 1000 + ru.ru_utime.tv_usec / 1000
              + ru.ru_stime.tv_sec * 1000 + ru.ru_stime.tv_usec / 1000;
    r.mem_kb  = ru.ru_maxrss;                          // Linux 上单位是 KB

    if (killed) { r.kind = 1; return r; }             // 墙钟超时
    if (WIFSIGNALED(status)) {
        r.sig = WTERMSIG(status);
        if (r.sig == SIGXCPU || r.sig == SIGKILL) { r.kind = 1; return r; }  // CPU 超时
        if (mem_mb > 0 && r.mem_kb > (long)mem_mb * 1024) { r.kind = 3; return r; }
        r.kind = 2; return r;                          // SIGSEGV/SIGABRT 等 -> RE
    }
    if (WEXITSTATUS(status) != 0) { r.kind = 2; return r; }                  // 非零退出 -> RE
    if (r.time_ms > time_ms)      { r.kind = 1; return r; }                  // 正常退出但 CPU 超时
    if (mem_mb > 0 && r.mem_kb > (long)mem_mb * 1024) { r.kind = 3; return r; }
    return r;                                          // kind = 0
}

// ---- 收集测试点(数字排序) ------------------------------------------------
static std::vector<std::pair<std::string,std::string>> collect_cases(const std::string& dir) {
    std::vector<std::pair<long,std::string>> ins;     // (序号, .in 路径)
    if (fs::is_directory(dir)) {
        for (auto& e : fs::directory_iterator(dir)) {
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

// ---- main ------------------------------------------------------------------
int main(int argc, char** argv) {
    std::string problem, src, lang = "cpp";
    int time_ms = 1000, mem_mb = 256;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> std::string { return (i + 1 < argc) ? std::string(argv[++i]) : std::string(); };
        if      (a == "--problem")  problem = next();
        else if (a == "--src")      src     = next();
        else if (a == "--lang")     lang    = next();
        else if (a == "--time-ms")  time_ms = std::atoi(next().c_str());
        else if (a == "--mem-mb")   mem_mb  = std::atoi(next().c_str());
        else if (a == "-h" || a == "--help") {
            printf("用法: judge --problem <dir> --src <file> --lang <cpp|python> "
                   "[--time-ms N] [--mem-mb N]\n");
            return 0;
        }
    }
    if (problem.empty() || src.empty()) {
        fprintf(stderr, "缺少 --problem 或 --src\n");
        emit("ERR", 0, 0, 0, 0, "参数缺失");
        return 2;
    }

    auto cases = collect_cases(problem);
    int total = (int)cases.size();
    if (total == 0) { emit("ERR", 0, 0, 0, 0, "题目目录无测试点"); return 0; }

    char tmpl[] = "/tmp/ojjudge.XXXXXX";
    if (!mkdtemp(tmpl)) { emit("ERR", 0, total, 0, 0, "无法创建临时目录"); return 0; }
    std::string tmp = tmpl;

    // 编译 / 准备运行命令
    std::vector<std::string> runcmd;
    if (lang == "cpp" || lang == "c++") {
        std::string bin = tmp + "/sol", cerr;
        if (!compile_cpp(src, bin, tmp, cerr)) {
            if (cerr.size() > 2000) cerr = cerr.substr(0, 2000) + "...";
            emit("CE", 0, total, 0, 0, cerr);
            fs::remove_all(tmp); return 0;
        }
        runcmd = { bin };
    } else if (lang == "python" || lang == "py") {
        std::string cerr;
        if (!pycheck(src, tmp, cerr)) {
            if (cerr.size() > 2000) cerr = cerr.substr(0, 2000) + "...";
            emit("CE", 0, total, 0, 0, cerr);
            fs::remove_all(tmp); return 0;
        }
        runcmd = { "python3", src };
    } else {
        emit("ERR", 0, total, 0, 0, "未知语言: " + lang);
        fs::remove_all(tmp); return 0;
    }

    // 逐测试点
    int passed = 0; long maxTime = 0, maxMem = 0;
    std::string status = "AC", detail = "";
    for (int i = 0; i < total; ++i) {
        std::string outf = tmp + "/out";
        RunResult r = run_one(runcmd, cases[i].first, outf, time_ms, mem_mb);
        maxTime = std::max(maxTime, r.time_ms);
        maxMem  = std::max(maxMem,  r.mem_kb);
        std::string tag = "case#" + std::to_string(i + 1);
        if (r.kind == 1) { status = "TLE"; detail = tag + " 超时"; break; }
        if (r.kind == 3) { status = "MLE"; detail = tag + " 超内存"; break; }
        if (r.kind == 2) { status = "RE";  detail = tag + " 运行错误(signal " + std::to_string(r.sig) + ")"; break; }
        if (r.kind == 4) { status = "ERR"; detail = tag + " 内部错误"; break; }
        if (normalize(read_file(outf)) == normalize(read_file(cases[i].second))) {
            passed++;
        } else { status = "WA"; detail = tag + " 输出不匹配"; break; }
    }

    emit(status, passed, total, maxTime, maxMem, detail);
    fs::remove_all(tmp);
    return 0;
}
