#!/usr/bin/env python3
# 把 mini-oj-docs 下文档融合成单文件 HTML 教程(套 article-to-html 模板 CSS)。
# 用法:cd mini-oj-docs && python3 build-tutorial.py
import subprocess, re, os

BASE = os.path.dirname(os.path.abspath(__file__))
TPL  = "/home/winbeau/.claude/skills/article-to-html/references/template.html"

css = re.search(r"<style>(.*?)</style>", open(TPL, encoding="utf-8").read(), re.S).group(1)

EXTRA = """
/* —— download buttons —— */
.dl-btn { display:inline-block; margin:4px 10px 4px 0; padding:9px 15px; font-family: ui-monospace, Menlo, monospace; font-size:12px; letter-spacing:0.02em; background: var(--accent); color:#fff; border:1px solid var(--accent); border-radius:7px; text-decoration:none; -webkit-font-smoothing:antialiased; transition: background .15s, border-color .15s; }
.dl-btn:hover { background:#5a87a4; border-color:#5a87a4; color:#fff; }
html.dark .dl-btn { color:#fff; }
/* —— interactivity —— */
section.collapsible h2 { cursor: pointer; position: relative; padding-right: 28px; }
section.collapsible h2::after { content:"\\2212"; position:absolute; right:8px; top:8px; font-family: ui-monospace, Menlo, monospace; color: var(--ink-faint); }
section.collapsible.collapsed h2::after { content:"+"; }
section.collapsible.collapsed > *:not(h2) { display:none; }
pre { position: relative; }
.copy-btn { position:absolute; top:7px; right:7px; font-family: ui-monospace, Menlo, monospace; font-size:10.5px; letter-spacing:0.08em; text-transform:uppercase; background: rgba(255,255,255,0.72); -webkit-backdrop-filter: blur(4px); backdrop-filter: blur(4px); border:1px solid var(--rule-soft); border-radius:6px; padding:4px 9px; cursor:pointer; color: var(--ink-faint); opacity:0.45; -webkit-font-smoothing:antialiased; transition: opacity .15s ease, color .15s, background .15s, border-color .15s; }
pre:hover .copy-btn { opacity:1; }
.copy-btn:hover { color: var(--ink); background:#fff; border-color: var(--rule); opacity:1; }
.copy-btn.copied { color:#fff; background: var(--accent); border-color: var(--accent); opacity:1; }
.toc { position: fixed; top: 56px; left: 20px; width: 230px; max-height: 82vh; overflow:auto; font-family: ui-monospace, Menlo, monospace; font-size: 11px; line-height: 1.65; padding-right:8px; }
.toc a { display:block; color: var(--ink-faint); border:none; padding: 1px 0; }
.toc a:hover { color: var(--ink-soft); }
.toc a.active { color: var(--accent); font-weight: 600; }
.toc .toc-title { color: var(--ink-soft); text-transform: uppercase; letter-spacing: 0.12em; margin-bottom: 8px; font-weight:600; }
@media (max-width: 1280px) { .toc { display:none; } }
html.dark { --paper:#1a1a1a; --paper-edge:#222; --ink:#e8e8e2; --ink-soft:#b8b8b2; --ink-faint:#888; --rule:#333; --rule-soft:#2a2a2a; --code-bg:#262624; --accent-faint:#1c2a35; --warn-soft:#2c2418; }
html.dark .callout, html.dark .card, html.dark figure { background:#1f1f1f; }
html.dark figcaption { background:#181818; }
html.dark pre, html.dark code { background:#262624; }
html.dark .copy-btn { background: rgba(38,38,36,0.72); border-color:#333; }
html.dark .copy-btn:hover { background:#2e2e2c; }
html.dark .copy-btn.copied { background: var(--accent); border-color: var(--accent); }
.theme-toggle { position: fixed; top:14px; right:14px; font-family: ui-monospace, Menlo, monospace; font-size:10px; letter-spacing:0.1em; text-transform:uppercase; background: var(--paper-edge); color: var(--ink-faint); border:1px solid var(--rule); padding:6px 10px; cursor:pointer; z-index:30; }
.reading-progress { position: fixed; top:0; left:0; height:2px; background: var(--accent); width:0%; z-index:40; transition: width 0.1s ease-out; }
"""

DOCS = [
    ("mini-oj-plan.md",              "学习路线图(三代主线)"),
    ("00-项目总设计.md",             "项目总设计 · 全局契约"),
    ("01-Ch1-环境与HelloWorld.md",   "第1章 · 环境与 HelloWorld(M0)"),
    ("02-Ch2-3-单题判题器.md",       "第2-3章 · 单题判题器(M1)"),
    ("03-Ch4-对象建模.md",           "第4章 · 对象建模(M2)"),
    ("04-Ch5-7-判题器与异常.md",     "第5-7章 · 接口/继承/多态/异常(M3·第一代)"),
    ("05-M4-第二代判题机.md",        "M4 · 第二代:反射+单文件(Ch8+Ch10)"),
    ("06-M5a-第三代判题机.md",       "M5a · 第三代:泛型集合+C++判题机(Ch15+Ch10)"),
    ("07-M5b-数据库衔接.md",         "M5b · 数据库衔接(Ch11)"),
    ("08-M5c-Swing客户端.md",        "M5c · Swing 大前端(Ch9)"),
    ("09-M6a-多线程收官.md",         "M6a · 多线程并发判题 · 收官(Ch12)"),
    ("judge/README.md",              "附录 · C++ 判题机使用手册"),
]

def convert(path):
    md = open(os.path.join(BASE, path), encoding="utf-8").read()
    lines, out, dropped = md.split("\n"), [], False
    for ln in lines:
        if not dropped and ln.startswith("# "):
            dropped = True
            continue
        out.append(ln)
    r = subprocess.run(
        ["pandoc", "-f", "gfm", "-t", "html",
         "--shift-heading-level-by=1", "--no-highlight"],
        input="\n".join(out), capture_output=True, text=True)
    if r.returncode != 0:
        raise SystemExit("pandoc failed on %s: %s" % (path, r.stderr))
    return r.stdout

sections = []
for i, (path, title) in enumerate(DOCS, 1):
    frag = convert(path)
    sections.append(
        '<section class="collapsible">\n'
        '<h2 id="sec-%d"><span class="num">%02d</span>%s</h2>\n%s\n</section>'
        % (i, i, title, frag))
SECTIONS = "\n\n".join(sections)

SVG = """<figure>
<svg viewBox="0 0 760 300" xmlns="http://www.w3.org/2000/svg" font-family="ui-monospace, Menlo, monospace" font-size="12">
  <defs><marker id="ah" markerWidth="10" markerHeight="8" refX="8" refY="3" orient="auto" markerUnits="strokeWidth"><path d="M0,0 L8,3 L0,6 Z" fill="#7a7a7a"/></marker></defs>
  <rect width="760" height="300" fill="#fff"/>
  <rect x="20"  y="52"  width="150" height="58" rx="4" fill="#fff"     stroke="#d8d8d2"/>
  <text x="95"  y="78"  text-anchor="middle" fill="#1a1a1a">Swing 客户端</text>
  <text x="95"  y="96"  text-anchor="middle" fill="#7a7a7a">Ch9 · 大前端</text>
  <rect x="295" y="52"  width="175" height="58" rx="4" fill="#fff"     stroke="#d8d8d2"/>
  <text x="382" y="78"  text-anchor="middle" fill="#1a1a1a">多线程判题队列</text>
  <text x="382" y="96"  text-anchor="middle" fill="#7a7a7a">Ch12 · 收官</text>
  <rect x="560" y="42"  width="180" height="78" rx="4" fill="#eef4f8"  stroke="#6f9bb8"/>
  <text x="650" y="68"  text-anchor="middle" fill="#1a1a1a">C++ 判题机 judge</text>
  <text x="650" y="88"  text-anchor="middle" fill="#4a4a4a">真编译·真运行</text>
  <text x="650" y="104" text-anchor="middle" fill="#4a4a4a">setrlimit 限资源</text>
  <rect x="295" y="206" width="175" height="54" rx="4" fill="#fff"     stroke="#d8d8d2"/>
  <text x="382" y="231" text-anchor="middle" fill="#1a1a1a">MySQL</text>
  <text x="382" y="249" text-anchor="middle" fill="#7a7a7a">Ch11 · 元数据/历史</text>
  <rect x="560" y="206" width="180" height="54" rx="4" fill="#fff"     stroke="#d8d8d2"/>
  <text x="650" y="231" text-anchor="middle" fill="#1a1a1a">文件题库 / 源码</text>
  <text x="650" y="249" text-anchor="middle" fill="#7a7a7a">Ch10</text>
  <line x1="170" y1="81" x2="293" y2="81" stroke="#7a7a7a" marker-end="url(#ah)"/>
  <text x="231" y="74"  text-anchor="middle" fill="#7a7a7a">提交</text>
  <line x1="470" y1="81" x2="558" y2="81" stroke="#7a7a7a" marker-end="url(#ah)"/>
  <text x="514" y="74"  text-anchor="middle" fill="#7a7a7a">调用</text>
  <line x1="382" y1="110" x2="382" y2="204" stroke="#7a7a7a" marker-end="url(#ah)"/>
  <text x="389" y="160" text-anchor="start"  fill="#7a7a7a">JDBC</text>
  <line x1="650" y1="206" x2="650" y2="122" stroke="#7a7a7a" marker-end="url(#ah)"/>
  <text x="657" y="166" text-anchor="start"  fill="#7a7a7a">题目/源码</text>
</svg>
<figcaption><span class="fig-num">FIG 1</span>最终架构(无网络) · Swing 大前端 → 多线程队列 → C++ 判题机;元数据/历史进 MySQL,测试点与源码留文件系统</figcaption>
</figure>"""

HTML = """<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width, initial-scale=1.0"/>
<title>用 Mini-OJ 项目学 Java · 判题机三代演进</title>
<style>
@@CSS@@
@@EXTRA@@
</style>
</head>
<body>
<div class="reading-progress" id="progress"></div>
<button class="theme-toggle" id="theme-toggle">dark</button>
<nav class="toc" id="toc"></nav>
<article class="doc">
<header class="doc-header">
  <div class="doc-eyebrow"><span class="mascot"></span>Mini-OJ · Java 学习教程</div>
  <h1 class="doc-title">用 Mini-OJ 项目学 Java</h1>
  <p class="doc-subtitle">判题机三代演进 · 古法手撸 · 教学优先</p>
  <div class="doc-meta">
    <span>STATUS · 教程</span>
    <span>ENV · Ubuntu + nvim + JDK</span>
    <span>REPO · github.com/winbeau/Mini-OJ-docs</span>
  </div>
</header>
<div class="tldr">
  <div class="tldr-label">TL;DR</div>
  以一个 <strong>Mini-OJ(在线判题系统)</strong> 为主线学完 Java 课程,贯穿<strong>判题机三代演进</strong>:第一代 Java 对象在 JVM 内模拟判题(M1–M3)、第二代反射工厂 + 单文件配置(M4)、第三代外部 <strong>C++ 判题机</strong>真编译/真运行/真 <code>setrlimit</code>(M5a)。其后用数据库(M5b)、Swing 大前端(M5c)、多线程并发(M6a 收官)做工程化。课程调整:<strong>第13章网络(Socket)已删</strong>,<strong>新增第15章泛型与集合</strong>。全程 <code>javac/java</code> 古法手撸。点击章节标题可折叠,右上角可切暗色。
</div>
@@FIGURE@@
@@SECTIONS@@
<footer>
  <h4>References</h4>
  <ul>
    <li>仓库 · <a href="https://github.com/winbeau/Mini-OJ-docs">github.com/winbeau/Mini-OJ-docs</a></li>
    <li>C++ 判题机源码 · <code>judge/judge.cpp</code>(编译型,真编译运行 C++/Python)</li>
    <li>本页由仓库内全部 Markdown 融合生成(build-tutorial.py)</li>
  </ul>
</footer>
</article>
<script>
(function(){
  var toc=document.getElementById('toc'); if(!toc) return;
  var t=document.createElement('div'); t.className='toc-title'; t.textContent='目录'; toc.appendChild(t);
  var hs=[].slice.call(document.querySelectorAll('section > h2'));
  hs.forEach(function(h){ var a=document.createElement('a'); a.href='#'+h.id; a.textContent=h.textContent.replace(/^\\s*\\d+\\s*/,''); toc.appendChild(a); });
  var links=[].slice.call(toc.querySelectorAll('a'));
  var obs=new IntersectionObserver(function(es){ es.forEach(function(e){ if(e.isIntersecting){ links.forEach(function(a){ a.classList.toggle('active', a.getAttribute('href')==='#'+e.target.id); }); } }); }, {rootMargin:'-30% 0px -60% 0px'});
  hs.forEach(function(s){ obs.observe(s); });
})();
document.querySelectorAll('section.collapsible h2').forEach(function(h){ h.addEventListener('click', function(e){ if(e.target.tagName==='A') return; h.parentElement.classList.toggle('collapsed'); }); });
document.querySelectorAll('pre').forEach(function(pre){ var b=document.createElement('button'); b.className='copy-btn'; b.textContent='copy'; b.onclick=async function(){ var c=pre.querySelector('code'); await navigator.clipboard.writeText(c?c.innerText:pre.innerText); b.textContent='copied'; b.classList.add('copied'); setTimeout(function(){ b.textContent='copy'; b.classList.remove('copied'); },1500); }; pre.appendChild(b); });
(function(){ var btn=document.getElementById('theme-toggle'); function apply(m){ document.documentElement.classList.toggle('dark', m==='dark'); btn.textContent = m==='dark'?'light':'dark'; } apply(localStorage.getItem('theme')||'light'); btn.addEventListener('click', function(){ var n=document.documentElement.classList.contains('dark')?'light':'dark'; localStorage.setItem('theme', n); apply(n); }); })();
(function(){ var p=document.getElementById('progress'); window.addEventListener('scroll', function(){ var h=document.documentElement.scrollHeight-innerHeight; p.style.width=(scrollY/Math.max(h,1))*100+'%'; }); })();
</script>
</body>
</html>"""

out = (HTML.replace("@@CSS@@", css)
           .replace("@@EXTRA@@", EXTRA)
           .replace("@@FIGURE@@", SVG)
           .replace("@@SECTIONS@@", SECTIONS))
dst = os.path.join(BASE, "mini-oj-tutorial.html")
open(dst, "w", encoding="utf-8").write(out)
print("wrote", dst, len(out), "bytes,", len(DOCS), "sections")
