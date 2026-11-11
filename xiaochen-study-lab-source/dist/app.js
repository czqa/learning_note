(function () {
  "use strict";

  const DATA = window.STUDY_DATA;
  const STORAGE_KEY = "xiaochen-study-lab-v1";
  const HIDDEN_NOTE_TRACKS = new Set([
    "Qt编程",
    "linux基础命令",
    "linux应用层开发",
    "linux环境工具链"
  ]);
  const $ = (selector, root = document) => root.querySelector(selector);
  const $$ = (selector, root = document) => [...root.querySelectorAll(selector)];
  const escapeHtml = (value = "") => value.replace(/[&<>"']/g, ch => ({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#39;"}[ch]));
  const slugify = (value, i) => "s-" + i + "-" + value.replace(/[^\w\u4e00-\u9fa5]+/g, "-").replace(/^-|-$/g, "").toLowerCase();
  const shuffle = array => {
    const result = [...array];
    for (let index = result.length - 1; index > 0; index--) {
      const target = Math.floor(Math.random() * (index + 1));
      [result[index], result[target]] = [result[target], result[index]];
    }
    return result;
  };

  const defaultState = {
    completed: [],
    mistakes: [],
    quizAnswered: 0,
    quizCorrect: 0,
    lastSection: null
  };
  let state = loadState();
  let noteSections = [];
  let noteTracks = [];
  let currentSectionIndex = 0;
  let cudaInterviewSections = [];
  let cudaInterviewArticles = [];
  let currentCudaInterviewIndex = 0;
  let currentRoute = "dashboard";
  const routeHistory = ["dashboard"];
  let cppLabEscapeHandler = null;
  let labResizeCleanup = null;
  let quiz = {items: [], index: 0, correct: 0, locked: false};
  let projects = Array.isArray(window.PROJECT_DATA?.projects) ? window.PROJECT_DATA.projects : [];
  let currentProjectIndex = 0;
  let projectResumeDepth = "brief";
  let aiStatus = {configured:false, signedIn:false, planEnabled:false, method:"none", model:"", provider:"未连接", account:null, accounts:[], apiKeyFallback:false, error:""};
  let aiModels = [];
  let projectInterview = {projectId:null, mode:"local", items:[], index:0, history:[], currentQuestion:null, sessionId:null, source:null, reasoningEffort:""};
  const projectResumeGenerationPending = new Set();
  const projectResumeModePending = new Set();
  const projectResumeReviewPending = new Set();
  let projectResumeAutoRun = null;
  let chatGptAuthSignal = 0;

  const clamp = (value, min, max) => Math.min(max, Math.max(min, value));
  const LAB_WIDTH_KEY = "xiaochen-lab-width-v1";
  const LAB_EDITOR_RATIO_KEY = "xiaochen-lab-editor-ratio-v1";
  const SIDEBAR_WIDTH_KEY = "xiaochen-sidebar-width-v1";
  const SIDEBAR_COLLAPSED_KEY = "xiaochen-sidebar-collapsed-v1";
  const CUDA_INTERVIEW_SIDEBAR_WIDTH_KEY = "xiaochen-cuda-interview-sidebar-width-v1";
  const CUDA_INTERVIEW_SIDEBAR_COLLAPSED_KEY = "xiaochen-cuda-interview-sidebar-collapsed-v1";
  const PROJECT_AI_MODEL_KEY = "xiaochen-project-ai-model-v1";
  const PROJECT_AI_EFFORT_KEY = "xiaochen-project-ai-effort-v1";
  const PROJECT_RESUME_DEPTH_KEY = "xiaochen-project-resume-depth-v1";
  const PROJECT_AI_EFFORT_LABELS = {none:"关闭", minimal:"最少", low:"低", medium:"中", high:"高", xhigh:"超高", max:"最高"};
  const PROJECT_AI_EFFORT_OPTIONS = {
    none:"关闭 · 最快",
    minimal:"最少 · 轻量",
    low:"低 · 更快",
    medium:"中 · 均衡",
    high:"高 · 更深入",
    xhigh:"超高 · 深度思考",
    max:"最高 · 质量优先"
  };
  try { projectResumeDepth = localStorage.getItem(PROJECT_RESUME_DEPTH_KEY) === "deep" ? "deep" : "brief"; }
  catch { projectResumeDepth = "brief"; }
  const CUDA_INTERVIEW_GROUP_LABELS = {
    "1 高频面试题总集":"第1章 高频面试题",
    "2 CUDA 基础与硬件模型":"第2章 基础与硬件",
    "3 内存、并发与执行优化":"第3章 内存与执行优化",
    "4 Profiling 与性能诊断":"第4章 性能分析",
    "5 手写 CUDA 算子":"第5章 手写 CUDA 算子",
    "6 GEMM、CUTLASS 与进阶优化":"第6章 GEMM 与进阶优化"
  };
  const CUDA_INTERVIEW_NAV_LABELS = {
    "CUDA 高频面试题 340 问：从硬件数值到 Blackwell 算子优化":"CUDA 340 题总集",
    "CUDA 硬件架构：SM、Warp、CUDA Core 与 Tensor Core":"GPU 硬件架构",
    "CUDA 线程模型：Grid、Block、Thread 与 Global Thread ID":"线程模型",
    "CUDA Warp 机制：分支分化、Active Mask 与 Warp 级原语":"Warp 机制",
    "CUDA Global Memory：合并访问、对齐与带宽优化":"全局内存",
    "CUDA Shared Memory：Bank Conflict、Padding 与 Swizzle":"共享内存",
    "CUDA Registers：寄存器使用、Spilling 与 Local Memory":"寄存器与本地内存",
    "CUDA Constant Memory 与 Texture Memory：只读数据的访存选择":"常量与纹理内存",
    "CUDA Streams：异步执行、Event 同步与计算传输重叠":"Stream 与 Event",
    "CUDA 循环展开：#pragma unroll 的原理与取舍":"循环展开",
    "CUDA Kernel Fusion：为什么融合能减少显存带宽瓶颈":"Kernel Fusion",
    "CUDA Profiling：Nsight Compute 与 Nsight Systems 常用指标":"NCU 与 NSYS",
    "Nsight Compute 指标全解：从 NCU 报告定位 CUDA Kernel 优化空间":"Nsight Compute 指标",
    "手写 CUDA Vector Add 与 Vector Dot Product":"Vector Add 与 Dot",
    "手写 CUDA Parallel Reduction：从交错寻址到 Warp 级规约":"Parallel Reduction",
    "手写 CUDA Softmax：Online Softmax 与数值稳定性":"Softmax",
    "手写 CUDA LayerNorm / RMSNorm：均值与方差的高效计算":"LayerNorm / RMSNorm",
    "手写 CUDA GEMM：朴素版、Shared Memory Tiling 与 Bank Conflict":"GEMM",
    "Split-K 详解：从 GEMM K 维并行到 Attention Split-KV":"Split-K",
    "CUTLASS 入门：从 CUDA Tiling 到可组合 GEMM Kernel":"CUTLASS",
    "CuTe DSL 入门：从 Layout、Tensor 到 GPU GEMM":"CuTe DSL",
    "LMDeploy Grouped GEMM 源码详解：从 C++ 调度到 CUTLASS WGMMA":"LMDeploy Grouped GEMM"
  };

  function loadState() {
    try {
      const loaded = {...defaultState, ...JSON.parse(localStorage.getItem(STORAGE_KEY) || "{}")};
      loaded.completed = Array.isArray(loaded.completed) ? loaded.completed : [];
      loaded.mistakes = Array.isArray(loaded.mistakes) ? loaded.mistakes : [];
      return loaded;
    }
    catch { return {...defaultState}; }
  }
  function saveState() {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(state));
  }

  function route(name, options = {}) {
    closeCppLab();
    if (options.back) {
      if (routeHistory.length > 1) routeHistory.pop();
      name = routeHistory[routeHistory.length - 1] || "dashboard";
    } else if (name !== currentRoute) {
      routeHistory.push(name);
    }
    currentRoute = name;
    $$(".view").forEach(view => view.classList.toggle("active", view.id === `${name}-view`));
    $$(".nav-item").forEach(item => item.classList.toggle("active", item.dataset.route === name));
    $("#back-button").hidden = name === "dashboard";
    window.scrollTo({top: 0, behavior: "smooth"});
    if (name === "mistakes") renderMistakes();
    $("#app").focus({preventScroll:true});
  }

  function parseNotes(markdown) {
    const lines = markdown.replace(/\r/g, "").split("\n");
    const sections = [];
    let current = null;
    let currentTrack = "总览";
    let inFence = false;

    lines.forEach((line, index) => {
      if (/^\s*```/.test(line)) inFence = !inFence;
      const match = !inFence && line.match(/^(#{1,3})\s+(.+?)\s*$/);
      if (match) {
        if (current) sections.push(current);
        const level = match[1].length;
        const title = cleanTitle(match[2]);
        if (level === 1) currentTrack = title;
        current = {id: slugify(title, sections.length), level, title, track: currentTrack, body: [], sourceLine: index + 1};
      } else if (current) {
        current.body.push(line);
      }
    });
    if (current) sections.push(current);
    return sections.map(section => ({...section, raw: section.body.join("\n").trim()}));
  }

  function cleanTitle(title) {
    return title.replace(/\*\*/g, "").replace(/`/g, "").trim();
  }

  function inlineMarkdown(text) {
    let html = escapeHtml(text);
    const codeTokens = [];
    html = html.replace(/`([^`]+)`/g, (_, code) => {
      const token = `@@CODETOKEN${codeTokens.length}@@`;
      codeTokens.push(`<code>${code}</code>`);
      return token;
    });
    html = html.replace(/!\[([^\]]*)\]\(([^)]+)\)/g, '<span class="note-image"><img src="$2" alt="$1" loading="lazy"><small>$1</small></span>');
    html = html.replace(/\[([^\]]+)\]\((https?:\/\/[^)]+)\)/g, '<a href="$2" target="_blank" rel="noreferrer">$1</a>');
    html = html.replace(/\*\*([^*]+)\*\*/g, "<strong>$1</strong>");
    html = html.replace(/==([^=]+)==/g, "<mark>$1</mark>");
    html = html.replace(/\*([^*]+)\*/g, "<em>$1</em>");
    codeTokens.forEach((code, index) => { html = html.replace(`@@CODETOKEN${index}@@`, code); });
    return html;
  }

  function markdownToHtml(markdown) {
    if (!markdown) return '<p class="muted">本节是章节目录，请从左侧选择具体小节。</p>';
    const lines = markdown.split("\n");
    const out = [];
    let i = 0;
    let listType = null;
    const closeList = () => { if (listType) { out.push(`</${listType}>`); listType = null; } };
    while (i < lines.length) {
      const line = lines[i];
      const fence = line.match(/^\s*```([^\s`]*)/);
      if (fence) {
        closeList();
        const code = [];
        const language = fence[1].toLowerCase();
        i++;
        while (i < lines.length && !/^\s*```/.test(lines[i])) code.push(lines[i++]);
        const content = code.join("\n");
        const isNarrativeText = ["text", "plain", "plaintext"].includes(language)
          && content.length > 100 && /[，。；！？]/.test(content) && !/[{};]/.test(content);
        if (isNarrativeText) {
          out.push(`<div class="note-explanation">${escapeHtml(content).replace(/\n/g, "<br>")}</div>`);
        } else {
          out.push(`<pre data-language="${escapeHtml(language || "code")}"><code>${escapeHtml(content)}</code></pre>`);
        }
        i++;
        continue;
      }
      if (line.includes("|") && i + 1 < lines.length && /^\s*\|?\s*:?-{3,}/.test(lines[i + 1])) {
        closeList();
        const header = splitTableRow(line);
        i += 2;
        const rows = [];
        while (i < lines.length && lines[i].includes("|") && lines[i].trim()) rows.push(splitTableRow(lines[i++]));
        out.push(`<table><thead><tr>${header.map(cell => `<th>${inlineMarkdown(cell)}</th>`).join("")}</tr></thead><tbody>${rows.map(row => `<tr>${row.map(cell => `<td>${inlineMarkdown(cell)}</td>`).join("")}</tr>`).join("")}</tbody></table>`);
        continue;
      }
      const heading = line.match(/^(#{4,6})\s+(.+)$/);
      if (heading) { closeList(); const level = Math.min(4, heading[1].length); out.push(`<h${level}>${inlineMarkdown(cleanTitle(heading[2]))}</h${level}>`); i++; continue; }
      const unordered = line.match(/^\s*[-*+]\s+(.+)$/);
      const ordered = line.match(/^\s*\d+[.)]\s+(.+)$/);
      if (unordered || ordered) {
        const wanted = unordered ? "ul" : "ol";
        if (listType !== wanted) { closeList(); out.push(`<${wanted}>`); listType = wanted; }
        out.push(`<li>${inlineMarkdown((unordered || ordered)[1])}</li>`);
        i++; continue;
      }
      closeList();
      if (!line.trim()) { i++; continue; }
      if (/^>\s?/.test(line)) { out.push(`<blockquote>${inlineMarkdown(line.replace(/^>\s?/, ""))}</blockquote>`); i++; continue; }
      if (/^[-*_]{3,}\s*$/.test(line)) { out.push("<hr>"); i++; continue; }
      if (/^<[^>]+>/.test(line.trim())) { out.push(`<p>${escapeHtml(line)}</p>`); i++; continue; }
      const paragraph = [line.trim()];
      i++;
      while (i < lines.length && lines[i].trim() && !/^\s*```/.test(lines[i]) && !/^(#{4,6})\s+/.test(lines[i]) && !/^\s*[-*+]\s+/.test(lines[i]) && !/^\s*\d+[.)]\s+/.test(lines[i]) && !/^>/.test(lines[i])) paragraph.push(lines[i++].trim());
      out.push(`<p>${inlineMarkdown(paragraph.join(" "))}</p>`);
    }
    closeList();
    return out.join("\n");
  }

  function splitTableRow(line) {
    let value = line.trim();
    if (value.startsWith("|")) value = value.slice(1);
    if (value.endsWith("|")) value = value.slice(0, -1);
    const cells = [];
    let cell = "";
    let inCode = false;
    for (let index = 0; index < value.length; index++) {
      const character = value[index];
      if (character === "\\" && value[index + 1] === "|") {
        cell += "|";
        index++;
        continue;
      }
      if (character === "`") {
        inCode = !inCode;
        cell += character;
        continue;
      }
      if (character === "|" && !inCode) {
        cells.push(cell.trim());
        cell = "";
        continue;
      }
      cell += character;
    }
    cells.push(cell.trim());
    return cells;
  }

  async function loadNotes() {
    try {
      let mainNote;
      let modernCppNote;
      let auxiliaryNote;
      let cudaInterviewNote;
      if (window.location.protocol === "file:" && window.STUDY_NOTE_DATA?.main) {
        mainNote = window.STUDY_NOTE_DATA.main;
        modernCppNote = window.STUDY_NOTE_DATA.modern || "";
        auxiliaryNote = window.STUDY_NOTE_DATA.auxiliary || "";
        cudaInterviewNote = window.STUDY_NOTE_DATA.cudaInterview || "";
      } else {
        const [mainResponse, modernResponse, auxiliaryResponse, cudaInterviewResponse] = await Promise.all([
          fetch("note.md"),
          fetch("modern-cpp.md").catch(() => null),
          fetch("aux-notes.md").catch(() => null),
          fetch("cuda-interview.md").catch(() => null)
        ]);
        if (!mainResponse.ok) throw new Error("笔记载入失败");
        mainNote = await mainResponse.text();
        modernCppNote = modernResponse?.ok ? await modernResponse.text() : "";
        auxiliaryNote = auxiliaryResponse?.ok ? await auxiliaryResponse.text() : "";
        cudaInterviewNote = cudaInterviewResponse?.ok ? await cudaInterviewResponse.text() : "";
      }
      const parsedSections = parseNotes(`${mainNote}\n\n${modernCppNote}\n\n${auxiliaryNote}`)
        .filter(section => !HIDDEN_NOTE_TRACKS.has(section.track));
      noteTracks = [...new Set(parsedSections.filter(s => s.level === 1).map(s => s.title))];
      const modernTrackIndex = noteTracks.indexOf("现代 C++");
      const designTrackIndex = noteTracks.indexOf("C++设计模式");
      if (modernTrackIndex >= 0 && designTrackIndex >= 0) {
        noteTracks.splice(modernTrackIndex, 1);
        noteTracks.splice(designTrackIndex + 1, 0, "现代 C++");
      }
      noteSections = noteTracks.flatMap(track => parsedSections.filter(section => section.track === track));
      cudaInterviewSections = parseNotes(cudaInterviewNote)
        .filter(section => section.track === "CUDA 面试篇");
      cudaInterviewArticles = cudaInterviewSections.filter(section => section.level === 3);
      renderTrackGrid();
      renderNotesTree();
      renderCudaInterviewTree();
      const storedIndex = noteSections.findIndex(section => section.id === state.lastSection);
      const firstLesson = noteSections.findIndex(section => section.level === 3 && section.raw);
      selectSection(storedIndex >= 0 ? storedIndex : Math.max(0, firstLesson));
      selectCudaInterviewArticle(0);
      $("#note-summary").textContent = `${noteTracks.length} 个方向 · ${noteSections.length} 个可读章节`;
    } catch (error) {
      $("#reader-title").textContent = "笔记载入失败";
      $("#reader-content").innerHTML = `<p>${escapeHtml(error.message)}，请稍后刷新重试。</p>`;
    }
  }

  function renderTrackGrid() {
    const icons = {
      "C++基础编程":"C++",
      "C++核心编程":"OOP",
      "C++提高编程":"STL",
      "C++11新特性":"11",
      "C++设计模式":"DP",
      "现代 C++":"MOD",
      "CUDA基础编程":"GPU",
      "CUDA提高编程":"OPT",
      "计算机网络":"NET",
      "数据结构与算法":"DS",
      "Linux 教程":"LIN"
    };
    const descriptions = {
      "C++基础编程":"语法、流程、数组、函数、指针与结构体",
      "C++核心编程":"内存、引用、类与对象、继承、多态与文件",
      "C++提高编程":"模板、STL 容器、函数对象与常用算法",
      "C++11新特性":"类型推导、lambda、智能指针与多线程",
      "C++设计模式":"设计原则、单例与工厂模式",
      "现代 C++":"类型推导、智能指针、移动语义与 Lambda",
      "CUDA基础编程":"线程模型、内存、数组、Thrust 与原子操作",
      "CUDA提高编程":"Reduce、SGEMM 与性能调优",
      "计算机网络":"分层模型、以太网、IP、TCP 与 UDP",
      "数据结构与算法":"表、栈、树、图、查找与排序",
      "Linux 教程":"命令与工具链、文件 I/O、进程线程与 Socket"
    };
    $("#track-grid").innerHTML = noteTracks.map((track, index) => {
      const count = noteSections.filter(section => section.track === track && section.level > 1).length;
      return `<article class="track-card" data-track="${escapeHtml(track)}" data-index="${String(index + 1).padStart(2, "0")}"><span class="track-icon">${icons[track] || "DEV"}</span><h3>${escapeHtml(track)}</h3><p>${descriptions[track] || `${count} 个章节`} · ${count} 节</p></article>`;
    }).join("");
    $$(".track-card").forEach(card => card.addEventListener("click", () => openTrack(card.dataset.track)));
  }

  function openTrack(track) {
    let index = noteSections.findIndex(section => section.track === track && section.level === 3 && section.raw);
    if (index < 0) index = noteSections.findIndex(section => section.track === track && section.level === 2);
    if (index >= 0) selectSection(index);
    route("notes");
  }

  function openCudaInterview() {
    if (cudaInterviewArticles.length) selectCudaInterviewArticle(currentCudaInterviewIndex);
    route("cuda-interview");
  }

  function cudaInterviewGroupFor(article) {
    const groups = cudaInterviewSections.filter(section => section.level === 2);
    return [...groups].reverse().find(group => group.sourceLine < article.sourceLine);
  }

  function renderCudaInterviewTree() {
    const root = $("#cuda-interview-tree");
    if (!root) return;
    const groups = cudaInterviewSections.filter(section => section.level === 2);
    root.innerHTML = groups.map((group, groupIndex) => {
      const nextGroup = groups[groupIndex + 1];
      const articles = cudaInterviewArticles.filter(article =>
        article.sourceLine > group.sourceLine
        && (!nextGroup || article.sourceLine < nextGroup.sourceLine)
      );
      const currentArticle = cudaInterviewArticles[currentCudaInterviewIndex];
      const groupLabel = CUDA_INTERVIEW_GROUP_LABELS[group.title] || group.title;
      return `<div class="cuda-outline-group"><div class="cuda-outline-group-title">${escapeHtml(groupLabel)}</div><div class="cuda-outline-items">${articles.map((article, articleIndex) => {
        const shortTitle = CUDA_INTERVIEW_NAV_LABELS[article.title] || article.title;
        const label = `${groupIndex + 1}.${articleIndex + 1} ${shortTitle}`;
        return `<button type="button" title="${escapeHtml(article.title)}" class="tree-item ${article.id === currentArticle?.id ? "active" : ""}" data-cuda-interview-id="${article.id}">${escapeHtml(label)}</button>`;
      }).join("")}</div></div>`;
    }).join("") || '<p class="muted">CUDA 面试内容载入失败。</p>';
    $$('[data-cuda-interview-id]', root).forEach(button => button.addEventListener("click", () => {
      const index = cudaInterviewArticles.findIndex(article => article.id === button.dataset.cudaInterviewId);
      selectCudaInterviewArticle(index);
      if (window.innerWidth < 700) $(".cuda-interview-reader").scrollIntoView({behavior:"smooth"});
    }));
  }

  function selectCudaInterviewArticle(index) {
    if (!cudaInterviewArticles.length || index < 0 || index >= cudaInterviewArticles.length) return;
    currentCudaInterviewIndex = index;
    const article = cudaInterviewArticles[index];
    const group = cudaInterviewGroupFor(article);
    $("#cuda-interview-title").textContent = article.title;
    $("#cuda-interview-breadcrumb").textContent = `CUDA 面试篇 · ${group?.title || "专题文章"}`;
    $("#cuda-interview-content").innerHTML = markdownToHtml(article.raw);
    const sourceUrl = article.raw.match(/\]\((https:\/\/hd9568\.github\.io\/[^)]+)\)/)?.[1]
      || "https://hd9568.github.io/topics/cuda/";
    $("#cuda-interview-source").href = sourceUrl;
    $("#cuda-interview-prev").disabled = index === 0;
    $("#cuda-interview-next").disabled = index === cudaInterviewArticles.length - 1;
    setupCodePresentation(article, "#cuda-interview-content");
    renderCudaInterviewTree();
  }

  function renderNotesTree() {
    const html = noteTracks.map(track => {
      const items = noteSections.filter(section => section.track === track && section.level > 1);
      if (!items.length) return "";
      const isOpen = noteSections[currentSectionIndex]?.track === track;
      return `<div class="tree-track ${isOpen ? "open" : ""}" data-tree-track="${escapeHtml(track)}"><button><b>${escapeHtml(track)}</b><span>${items.length} 节</span></button><div class="tree-items">${items.map(section => `<button class="tree-item level-${section.level} ${section.id === noteSections[currentSectionIndex]?.id ? "active" : ""} ${state.completed.includes(section.id) ? "completed" : ""}" data-section-id="${section.id}">${escapeHtml(section.title)}</button>`).join("")}</div></div>`;
    }).join("");
    $("#notes-tree").innerHTML = html || '<p class="muted">没有匹配的章节。</p>';
    $$(".tree-track > button").forEach(button => button.addEventListener("click", () => button.parentElement.classList.toggle("open")));
    $$(".tree-item").forEach(button => button.addEventListener("click", () => {
      const index = noteSections.findIndex(section => section.id === button.dataset.sectionId);
      selectSection(index);
      if (window.innerWidth < 700) $(".note-reader").scrollIntoView({behavior:"smooth"});
    }));
  }

  function selectSection(index) {
    if (!noteSections.length || index < 0 || index >= noteSections.length) return;
    currentSectionIndex = index;
    const section = noteSections[index];
    $(".note-reader").classList.toggle("modern-cpp-reader", section.track === "现代 C++");
    state.lastSection = section.id;
    saveState();
    $("#reader-title").textContent = section.title;
    $("#reader-breadcrumb").textContent = `${section.track} · 第 ${section.sourceLine} 行`;
    $("#reader-content").innerHTML = markdownToHtml(section.raw);
    setupCppExamples(section);
    setupCodePresentation(section);
    const button = $("#complete-section");
    const done = state.completed.includes(section.id);
    button.classList.toggle("done", done);
    button.textContent = done ? "✓ 已完成" : "标记已完成";
    $("#prev-section").disabled = index === 0;
    $("#next-section").disabled = index === noteSections.length - 1;
    renderNotesTree();
  }

  function toggleComplete() {
    const id = noteSections[currentSectionIndex]?.id;
    if (!id) return;
    const isCompleted = state.completed.includes(id);
    state.completed = isCompleted ? state.completed.filter(item => item !== id) : [...state.completed, id];
    saveState();
    selectSection(currentSectionIndex);
  }

  function setupCppExamples(section) {
    if (/^CUDA/i.test(section.track)) return;
    const codeBlocks = $$("#reader-content pre code").filter(code => code.parentElement.dataset.language !== "text" && isRunnableCpp(code.textContent));
    if (!codeBlocks.length) return;
    codeBlocks.forEach((code, index) => {
      const pre = code.parentElement;
      const wrapper = document.createElement("div");
      wrapper.className = "code-example";
      pre.before(wrapper);
      wrapper.append(pre);
      const toolbar = document.createElement("div");
      toolbar.className = "code-example-toolbar";
      toolbar.innerHTML = `<span><b>C++</b> 示例 ${index + 1}</span><div class="code-example-actions"><button class="code-copy" type="button">复制代码</button><button class="code-run" type="button">C++ 运行 ↗</button></div>`;
      wrapper.prepend(toolbar);
      $(".code-copy", toolbar).addEventListener("click", event => copyCode(code.textContent, event.currentTarget));
      $(".code-run", toolbar).addEventListener("click", () => openCppLab(section, index, code.textContent));
    });
  }

  function setupCodePresentation(section, contentSelector = "#reader-content") {
    const isCudaSection = /^CUDA/i.test(section.track);
    const contentRoot = $(contentSelector);
    if (!contentRoot) return;
    const codeBlocks = $$("pre code", contentRoot);
    codeBlocks.forEach((code, index) => {
      const pre = code.parentElement;
      let wrapper = pre.closest(".code-example");
      let toolbar = wrapper ? $(".code-example-toolbar", wrapper) : null;

      if (!wrapper) {
        const language = String(pre.dataset.language || "code").toLowerCase();
        const languageLabel = isCudaSection ? "CUDA" : (/^(?:c\+\+|cpp|cxx)$/.test(language) ? "C++" : language.toUpperCase());
        wrapper = document.createElement("div");
        wrapper.className = "code-example";
        pre.before(wrapper);
        wrapper.append(pre);
        toolbar = document.createElement("div");
        toolbar.className = "code-example-toolbar";
        toolbar.innerHTML = `<span><b>${escapeHtml(languageLabel)}</b> 代码 ${index + 1}</span><div class="code-example-actions"><button class="code-copy" type="button">复制代码</button></div>`;
        wrapper.prepend(toolbar);
        $(".code-copy", toolbar).addEventListener("click", event => copyCode(code.textContent, event.currentTarget));
      }

      if (isCudaSection && !$(".code-runtime-note", toolbar)) {
        const runtimeNote = document.createElement("small");
        runtimeNote.className = "code-runtime-note";
        runtimeNote.textContent = "本机 NVIDIA GPU";
        $(".code-example-toolbar > span", toolbar).append(runtimeNote);
      }

      if (isCudaSection && isRunnableCuda(code.textContent) && !$(".cuda-run", toolbar)) {
        const runButton = document.createElement("button");
        runButton.className = "code-run cuda-run";
        runButton.type = "button";
        runButton.textContent = "CUDA 运行 ↗";
        runButton.addEventListener("click", () => openCudaLab(section, index, code.textContent));
        $(".code-example-actions", toolbar).append(runButton);
      }

      const lineCount = code.textContent.replace(/\n+$/, "").split("\n").length;
      if (lineCount > 24 && !$(".code-expand", toolbar)) {
        wrapper.classList.add("is-collapsible");
        const expandButton = document.createElement("button");
        expandButton.className = "code-expand";
        expandButton.type = "button";
        expandButton.textContent = "展开代码";
        expandButton.addEventListener("click", () => {
          const expanded = wrapper.classList.toggle("is-expanded");
          expandButton.textContent = expanded ? "收起代码" : "展开代码";
          expandButton.setAttribute("aria-expanded", String(expanded));
        });
        expandButton.setAttribute("aria-expanded", "false");
        $(".code-example-actions", toolbar).prepend(expandButton);
      }
    });
  }

  function isRunnableCpp(value) {
    const code = value.trim();
    return /\b(?:int|signed|auto)\s+main\s*\(/.test(code)
      && /[{}]/.test(code)
      && !/(?:__global__|cuda_runtime\.h|cudaMalloc\s*\()/.test(code);
  }

  function isRunnableCuda(value) {
    const code = value.trim();
    return /\b(?:int|auto)\s+main\s*\(/.test(code)
      && /(?:__global__|cuda_runtime\.h|cudaMalloc\s*\()/.test(code);
  }

  async function copyCode(value, button) {
    const originalLabel = button.textContent;
    try {
      if (navigator.clipboard && window.isSecureContext) {
        await navigator.clipboard.writeText(value);
      } else {
        const temporary = document.createElement("textarea");
        temporary.value = value;
        temporary.style.cssText = "position:fixed;left:-9999px;top:0;opacity:0";
        document.body.append(temporary);
        temporary.select();
        document.execCommand("copy");
        temporary.remove();
      }
      button.textContent = "✓ 已复制";
    } catch {
      button.textContent = "复制失败";
    }
    setTimeout(() => { if (button.isConnected) button.textContent = originalLabel; }, 1400);
  }

  function closeCppLab() {
    if (labResizeCleanup) {
      labResizeCleanup();
      labResizeCleanup = null;
    }
    $("#cpp-lab")?.remove();
    document.body.classList.remove("lab-open", "lab-resizing", "lab-resizing-x", "lab-resizing-y");
    if (cppLabEscapeHandler) {
      document.removeEventListener("keydown", cppLabEscapeHandler);
      cppLabEscapeHandler = null;
    }
  }

  function openCppLab(section, index, originalCode) {
    openCodeLab(section, index, originalCode, "cpp");
  }

  function openCudaLab(section, index, originalCode) {
    openCodeLab(section, index, originalCode, "cuda");
  }

  function openCodeLab(section, index, originalCode, mode) {
    closeCppLab();
    const isCuda = mode === "cuda";
    const storageKey = `xiaochen-${mode}-lab:${section.id}:${index}`;
    const savedCode = localStorage.getItem(storageKey);
    const dock = document.createElement("aside");
    dock.id = "cpp-lab";
    dock.className = "cpp-lab-dock";
    const standardOptions = isCuda
      ? '<option value="c++11">C++11</option><option value="c++14">C++14</option><option value="c++17" selected>C++17</option><option value="c++20">C++20</option>'
      : '<option value="c++11">C++11</option><option value="c++14">C++14</option><option value="c++17" selected>C++17</option><option value="c++20">C++20</option>';
    dock.innerHTML = `
      <div class="lab-width-resizer" role="separator" aria-label="拖动调整正文和工作台宽度" aria-orientation="vertical" tabindex="0" title="左右拖动调整宽度，双击恢复默认"></div>
      <section class="cpp-lab" role="region" aria-labelledby="cpp-lab-title">
      <header class="lab-head">
        <div class="lab-head-copy"><p class="eyebrow">${isCuda ? "CUDA GPU LAB" : "LOCAL C++ LAB"}</p><h3 id="cpp-lab-title">${isCuda ? "本机 CUDA 工作台" : "本机 C++ 工作台"}</h3><small>${escapeHtml(section.title)} · 示例 ${index + 1}</small></div>
        <button class="lab-close" type="button" aria-label="关闭${isCuda ? "本机 CUDA" : "本机 C++"}工作台">×</button>
      </header>
      <div class="lab-toolbar">
        <label><span>语言标准</span><select id="cpp-standard">${standardOptions}</select></label>
        <div class="lab-toolbar-actions"><button class="lab-copy" type="button">复制代码</button><button class="lab-reset" type="button">恢复示例</button><button class="lab-run" type="button">▶ 运行代码 <kbd>Ctrl ↵</kbd></button></div>
      </div>
      <div class="lab-workspace">
        <div class="lab-editor-panel">
          <div class="lab-panel-head"><span>代码编辑器</span><small>修改内容会自动保存在当前浏览器</small></div>
          <textarea id="cpp-editor" class="cpp-editor" spellcheck="false" aria-label="C++ 代码编辑器"></textarea>
        </div>
        <div class="lab-height-resizer" role="separator" aria-label="拖动调整代码和运行结果高度" aria-orientation="horizontal" tabindex="0" title="上下拖动调整高度，双击恢复默认"></div>
        <aside class="lab-side-panel">
          <div class="lab-output-panel">
            <div class="lab-output-head"><span>运行结果</span><small>${isCuda ? "由本机 nvcc 编译并在 NVIDIA GPU 上执行" : "由本机 g++ 编译执行，支持 Linux 系统调用"}</small></div>
            <pre id="cpp-output" class="cpp-output" aria-live="polite">准备就绪。\n\n修改左侧代码，再点击“运行代码”。</pre>
          </div>
        </aside>
      </div>
      </section>`;
    document.body.append(dock);
    document.body.classList.add("lab-open");
    labResizeCleanup = setupLabResizers(dock);
    const editor = $("#cpp-editor", dock);
    const runButton = $(".lab-run", dock);
    const runCode = () => isCuda ? compileCuda(dock) : compileCpp(dock);
    editor.value = savedCode ?? originalCode;
    editor.addEventListener("input", () => localStorage.setItem(storageKey, editor.value));
    editor.addEventListener("keydown", event => {
      if (event.key === "Tab") {
        event.preventDefault();
        const start = editor.selectionStart;
        editor.setRangeText("    ", start, editor.selectionEnd, "end");
        localStorage.setItem(storageKey, editor.value);
      }
      if (event.key === "Enter" && (event.ctrlKey || event.metaKey)) runCode();
    });
    $(".lab-copy", dock).addEventListener("click", event => copyCode(editor.value, event.currentTarget));
    $(".lab-reset", dock).addEventListener("click", () => {
      editor.value = originalCode;
      localStorage.removeItem(storageKey);
      editor.focus();
    });
    $(".lab-close", dock).addEventListener("click", closeCppLab);
    cppLabEscapeHandler = event => { if (event.key === "Escape") closeCppLab(); };
    document.addEventListener("keydown", cppLabEscapeHandler);
    runButton.addEventListener("click", runCode);
    setTimeout(() => editor.focus({preventScroll:true}), 50);
  }

  function setupLabResizers(dock) {
    const widthHandle = $(".lab-width-resizer", dock);
    const heightHandle = $(".lab-height-resizer", dock);
    const workspace = $(".lab-workspace", dock);
    let editorRatio = Number.parseFloat(localStorage.getItem(LAB_EDITOR_RATIO_KEY));
    if (!Number.isFinite(editorRatio)) editorRatio = .62;
    let dockWidth = Number.parseFloat(localStorage.getItem(LAB_WIDTH_KEY));
    if (!Number.isFinite(dockWidth)) dockWidth = Math.min(620, window.innerWidth * .48);

    const setDockWidth = value => {
      const min = Math.min(360, window.innerWidth);
      const max = Math.max(min, Math.min(900, window.innerWidth - 300));
      dockWidth = clamp(value, min, max);
      document.body.style.setProperty("--lab-dock-width", `${Math.round(dockWidth)}px`);
      widthHandle.setAttribute("aria-valuenow", String(Math.round(dockWidth)));
      widthHandle.setAttribute("aria-valuemin", String(Math.round(min)));
      widthHandle.setAttribute("aria-valuemax", String(Math.round(max)));
      return dockWidth;
    };
    const setEditorRatio = ratio => {
      const available = Math.max(1, workspace.getBoundingClientRect().height - 8);
      const minEditor = Math.min(180, available * .48);
      const minOutput = Math.min(130, available * .4);
      const editorHeight = clamp(available * ratio, minEditor, Math.max(minEditor, available - minOutput));
      editorRatio = editorHeight / available;
      workspace.style.gridTemplateRows = `${Math.round(editorHeight)}px 8px minmax(0, 1fr)`;
      heightHandle.setAttribute("aria-valuenow", String(Math.round(editorRatio * 100)));
      heightHandle.setAttribute("aria-valuemin", "20");
      heightHandle.setAttribute("aria-valuemax", "80");
      return editorRatio;
    };
    const stopResize = () => {
      document.body.classList.remove("lab-resizing", "lab-resizing-x", "lab-resizing-y");
    };

    const startWidthResize = event => {
      if (window.innerWidth <= 700 || event.button !== 0) return;
      event.preventDefault();
      widthHandle.setPointerCapture(event.pointerId);
      document.body.classList.add("lab-resizing", "lab-resizing-x");
    };
    const moveWidthResize = event => {
      if (!widthHandle.hasPointerCapture(event.pointerId)) return;
      setDockWidth(window.innerWidth - event.clientX);
    };
    const endWidthResize = event => {
      if (!widthHandle.hasPointerCapture(event.pointerId)) return;
      widthHandle.releasePointerCapture(event.pointerId);
      localStorage.setItem(LAB_WIDTH_KEY, String(Math.round(dockWidth)));
      stopResize();
    };
    const startHeightResize = event => {
      if (event.button !== 0) return;
      event.preventDefault();
      heightHandle.setPointerCapture(event.pointerId);
      document.body.classList.add("lab-resizing", "lab-resizing-y");
    };
    const moveHeightResize = event => {
      if (!heightHandle.hasPointerCapture(event.pointerId)) return;
      const rect = workspace.getBoundingClientRect();
      setEditorRatio((event.clientY - rect.top) / Math.max(1, rect.height - 8));
    };
    const endHeightResize = event => {
      if (!heightHandle.hasPointerCapture(event.pointerId)) return;
      heightHandle.releasePointerCapture(event.pointerId);
      localStorage.setItem(LAB_EDITOR_RATIO_KEY, String(editorRatio));
      stopResize();
    };
    const onWidthKey = event => {
      if (event.key !== "ArrowLeft" && event.key !== "ArrowRight") return;
      event.preventDefault();
      setDockWidth(dockWidth + (event.key === "ArrowLeft" ? 32 : -32));
      localStorage.setItem(LAB_WIDTH_KEY, String(Math.round(dockWidth)));
    };
    const onHeightKey = event => {
      if (event.key !== "ArrowUp" && event.key !== "ArrowDown") return;
      event.preventDefault();
      setEditorRatio(editorRatio + (event.key === "ArrowDown" ? .04 : -.04));
      localStorage.setItem(LAB_EDITOR_RATIO_KEY, String(editorRatio));
    };
    const resetWidth = () => {
      localStorage.removeItem(LAB_WIDTH_KEY);
      setDockWidth(Math.min(620, window.innerWidth * .48));
    };
    const resetHeight = () => {
      localStorage.removeItem(LAB_EDITOR_RATIO_KEY);
      setEditorRatio(.62);
    };
    const onWindowResize = () => {
      setDockWidth(dockWidth);
      setEditorRatio(editorRatio);
    };

    widthHandle.addEventListener("pointerdown", startWidthResize);
    widthHandle.addEventListener("pointermove", moveWidthResize);
    widthHandle.addEventListener("pointerup", endWidthResize);
    widthHandle.addEventListener("pointercancel", stopResize);
    widthHandle.addEventListener("keydown", onWidthKey);
    widthHandle.addEventListener("dblclick", resetWidth);
    heightHandle.addEventListener("pointerdown", startHeightResize);
    heightHandle.addEventListener("pointermove", moveHeightResize);
    heightHandle.addEventListener("pointerup", endHeightResize);
    heightHandle.addEventListener("pointercancel", stopResize);
    heightHandle.addEventListener("keydown", onHeightKey);
    heightHandle.addEventListener("dblclick", resetHeight);
    window.addEventListener("resize", onWindowResize);
    setDockWidth(dockWidth);
    requestAnimationFrame(() => setEditorRatio(editorRatio));

    return () => {
      window.removeEventListener("resize", onWindowResize);
      stopResize();
    };
  }

  async function compileCpp(lab) {
    const editor = $("#cpp-editor", lab);
    const output = $("#cpp-output", lab);
    const runButton = $(".lab-run", lab);
    const standard = $("#cpp-standard", lab).value;
    if (!editor.value.trim()) { output.textContent = "请先输入 C++ 代码。"; return; }
    runButton.disabled = true;
    runButton.textContent = "正在检查 g++…";
    output.className = "cpp-output running";
    output.textContent = "正在连接本机 C++ 服务…";
    try {
      const statusResponse = await fetch(localRunnerUrl("/api/cpp/status"), {headers:{Accept:"application/json"}});
      const status = await statusResponse.json().catch(() => ({}));
      if (!statusResponse.ok || !status.available || !status.token) throw new Error(status.message || "本机 g++ 暂不可用");
      runButton.textContent = "正在编译…";
      output.textContent = `${status.compiler}\n\n正在编译并运行 C++ 代码…`;
      const response = await fetch(localRunnerUrl("/api/cpp/run"), {
        method: "POST",
        headers:{"Content-Type":"application/json", Accept:"application/json", "X-CUDA-Token":status.token},
        body: JSON.stringify({source:editor.value, standard})
      });
      const data = await response.json().catch(() => ({}));
      if (!response.ok) throw new Error(data.error || `C++ 服务返回 ${response.status}`);
      const compilation = data.compilation || {};
      const execution = data.execution || {};
      const pieces = [];
      if (compilation.stderr) pieces.push(`【编译信息】\n${compilation.stderr}`);
      if (compilation.stdout) pieces.push(`【编译输出】\n${compilation.stdout}`);
      if (data.phase === "compile") {
        if (compilation.timedOut) pieces.push("【编译失败】编译超过 20 秒，已停止。");
        else if (compilation.truncated) pieces.push("【编译失败】编译输出过长，已停止。");
        else if (!compilation.stderr && !compilation.stdout) pieces.push("【编译失败】请检查 C++ 代码。");
      } else {
        if (execution.stdout) pieces.push(`【程序输出】\n${execution.stdout}`);
        if (execution.stderr) pieces.push(`【运行信息】\n${execution.stderr}`);
        if (execution.timedOut) pieces.push("【运行停止】程序运行超过 10 秒。");
        else if (execution.truncated) pieces.push("【运行停止】程序输出过长。");
      }
      output.className = `cpp-output ${data.ok ? "success" : "error"}`;
      output.textContent = pieces.join("\n\n").trim() || (data.ok ? "C++ 程序运行成功，没有文本输出。" : "C++ 编译或运行失败。");
    } catch (error) {
      output.className = "cpp-output error";
      output.textContent = `C++ 暂时无法运行：${error.message}`;
    } finally {
      runButton.disabled = false;
      runButton.innerHTML = "▶ 运行代码 <kbd>Ctrl ↵</kbd>";
    }
  }

  function localRunnerUrl(path) {
    const origin = window.location.protocol === "file:" ? "http://127.0.0.1:8765" : "";
    return `${origin}${path}`;
  }

  async function redirectFileModeToLocalServer() {
    if (window.location.protocol !== "file:") return false;
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), 1800);
    try {
      await fetch("http://127.0.0.1:8765/api/ai/status", {
        mode:"no-cors",
        cache:"no-store",
        signal:controller.signal
      });
      window.location.replace("http://127.0.0.1:8765/");
      return true;
    } catch {
      return false;
    } finally {
      clearTimeout(timeout);
    }
  }

  async function compileCuda(lab) {
    const editor = $("#cpp-editor", lab);
    const output = $("#cpp-output", lab);
    const runButton = $(".lab-run", lab);
    const standard = $("#cpp-standard", lab).value;
    if (!editor.value.trim()) { output.textContent = "请先输入 CUDA 代码。"; return; }
    runButton.disabled = true;
    runButton.textContent = "正在检查 GPU…";
    output.className = "cpp-output running";
    output.textContent = "正在连接本机 CUDA 服务…";
    try {
      const statusResponse = await fetch(localRunnerUrl("/api/cuda/status"), {headers:{Accept:"application/json"}});
      const status = await statusResponse.json().catch(() => ({}));
      if (!statusResponse.ok || !status.available || !status.token) throw new Error(status.message || "本机 CUDA 环境暂不可用");
      runButton.textContent = "正在编译…";
      output.textContent = `${status.gpu}\n\n正在使用 nvcc 编译 CUDA 代码…`;
      const response = await fetch(localRunnerUrl("/api/cuda/run"), {
        method: "POST",
        headers:{"Content-Type":"application/json", Accept:"application/json", "X-CUDA-Token":status.token},
        body: JSON.stringify({source:editor.value, standard})
      });
      const data = await response.json().catch(() => ({}));
      if (!response.ok) throw new Error(data.error || `CUDA 服务返回 ${response.status}`);
      const pieces = [];
      const compilation = data.compilation || {};
      const execution = data.execution || {};
      if (compilation.stderr) pieces.push(`【编译信息】\n${compilation.stderr}`);
      if (compilation.stdout) pieces.push(`【编译输出】\n${compilation.stdout}`);
      if (data.phase === "compile") {
        if (compilation.timedOut) pieces.push("【编译失败】编译超过 30 秒，已停止。");
        else if (compilation.truncated) pieces.push("【编译失败】编译输出过长，已停止。");
        else if (!compilation.stderr && !compilation.stdout) pieces.push("【编译失败】请检查 CUDA 代码。");
      } else {
        if (execution.stdout) pieces.push(`【程序输出】\n${execution.stdout}`);
        if (execution.stderr) pieces.push(`【运行信息】\n${execution.stderr}`);
        if (execution.timedOut) pieces.push("【运行停止】程序运行超过 10 秒。");
        else if (execution.truncated) pieces.push("【运行停止】程序输出过长。");
      }
      output.className = `cpp-output ${data.ok ? "success" : "error"}`;
      output.textContent = pieces.join("\n\n").trim() || (data.ok ? "CUDA 程序运行成功，没有文本输出。" : "CUDA 编译或运行失败。");
    } catch (error) {
      output.className = "cpp-output error";
      const unavailable = /fetch|network|连接|Failed/i.test(error.message);
      output.textContent = unavailable
        ? `CUDA 服务尚未启动。\n\n请双击项目目录中的“启动学习网站.desktop”，再回到 CUDA 代码运行。\n\n详细信息：${error.message}`
        : `CUDA 暂时无法运行：${error.message}`;
    } finally {
      runButton.disabled = false;
      runButton.innerHTML = "▶ 运行代码 <kbd>Ctrl ↵</kbd>";
    }
  }

  function setupCategories() {
    const options = DATA.categories.map(category => `<option value="${category}">${category}</option>`).join("");
    $("#quiz-category").innerHTML = options;
  }

  function isShortAnswerQuestion(question) {
    return question?.type === "short";
  }

  function startQuiz(quick = false) {
    const category = quick ? "全部" : $("#quiz-category").value;
    const type = quick ? "all" : $("#quiz-type").value;
    const countValue = quick ? "10" : $("#quiz-count").value;
    const categoryPool = category === "全部" ? DATA.questions : DATA.questions.filter(question => question.category === category);
    const pool = type === "all"
      ? categoryPool
      : categoryPool.filter(question => type === "short" ? isShortAnswerQuestion(question) : !isShortAnswerQuestion(question));
    const count = countValue === "all" ? pool.length : Math.min(Number(countValue), pool.length);
    quiz = {items: shuffle(pool).slice(0, count), index: 0, correct: 0, locked: false};
    $("#quiz-welcome").classList.add("hidden");
    $("#quiz-result").classList.add("hidden");
    $("#quiz-session").classList.remove("hidden");
    route("quiz");
    renderQuestion();
  }

  function renderQuestion() {
    const question = quiz.items[quiz.index];
    if (!question) return finishQuiz();
    quiz.locked = false;
    $("#quiz-number").textContent = `第 ${quiz.index + 1} / ${quiz.items.length} 题`;
    $("#quiz-score").textContent = `已掌握 ${quiz.correct} 题`;
    $("#quiz-progress-bar").style.width = `${(quiz.index / quiz.items.length) * 100}%`;
    $("#quiz-tag").textContent = `${question.category} · ${isShortAnswerQuestion(question) ? "简答题" : "选择题"}`;
    $("#quiz-question").textContent = question.q;
    if (isShortAnswerQuestion(question)) {
      $("#quiz-options").innerHTML = `<div class="short-answer-box"><label for="short-answer-input">先用自己的话回答</label><textarea id="short-answer-input" placeholder="建议按“结论 → 原因/机制 → 场景或取舍”组织，至少写出一个完整要点。"></textarea><div class="short-answer-actions"><small id="short-answer-status">写完后再查看参考要点，不会按关键词机械判分。</small><button class="primary-button compact" id="reveal-short-answer" type="button">查看参考要点</button></div></div>`;
      $("#reveal-short-answer").addEventListener("click", revealShortAnswer);
    } else {
      $("#quiz-options").innerHTML = question.options.map((option, index) => `<button class="option" data-option="${index}"><span>${String.fromCharCode(65 + index)}</span>${escapeHtml(option)}</button>`).join("");
      $$(".option").forEach(button => button.addEventListener("click", () => answerQuestion(Number(button.dataset.option))));
    }
    $("#quiz-explanation").classList.add("hidden");
    $("#quiz-explanation").innerHTML = "";
    $("#next-question").classList.add("hidden");
  }

  function recordQuizOutcome(question, mastered) {
    if (mastered) quiz.correct++;
    state.quizAnswered++;
    if (mastered) state.quizCorrect++;
    if (!mastered && !state.mistakes.includes(question.id)) state.mistakes.push(question.id);
    if (mastered) state.mistakes = state.mistakes.filter(id => id !== question.id);
    saveState();
  }

  function answerQuestion(choice) {
    if (quiz.locked) return;
    quiz.locked = true;
    const question = quiz.items[quiz.index];
    const correct = choice === question.answer;
    recordQuizOutcome(question, correct);
    $$(".option").forEach((button, index) => {
      button.disabled = true;
      if (index === question.answer) button.classList.add("correct");
      if (index === choice && !correct) button.classList.add("wrong");
    });
    $("#quiz-explanation").innerHTML = `<strong>${correct ? "回答正确" : `正确答案：${String.fromCharCode(65 + question.answer)}`}</strong><br>${escapeHtml(question.why)}`;
    $("#quiz-explanation").classList.remove("hidden");
    $("#next-question").textContent = quiz.index === quiz.items.length - 1 ? "查看结果 →" : "下一题 →";
    $("#next-question").classList.remove("hidden");
  }

  function revealShortAnswer() {
    if (quiz.locked) return;
    const question = quiz.items[quiz.index];
    const textarea = $("#short-answer-input");
    const answer = textarea.value.trim();
    if (answer.length < 10) {
      textarea.classList.add("invalid");
      $("#short-answer-status").textContent = "请先写下自己的思路（至少 10 个字），再查看参考要点。";
      textarea.focus();
      return;
    }
    textarea.classList.remove("invalid");
    textarea.readOnly = true;
    $("#reveal-short-answer").disabled = true;
    const points = (question.points || []).map(point => `<li>${escapeHtml(point)}</li>`).join("");
    const sample = question.sample ? `<details class="short-answer-sample"><summary>展开示范回答</summary><p>${escapeHtml(question.sample)}</p></details>` : "";
    $("#quiz-explanation").innerHTML = `<strong>参考要点</strong><ul>${points}</ul>${sample}<div class="short-answer-rating"><span>对照后，你能不看答案重新讲清楚吗？</span><button type="button" data-short-grade="review">还需复习</button><button type="button" data-short-grade="mastered">已经掌握</button></div>`;
    $("#quiz-explanation").classList.remove("hidden");
    $$('[data-short-grade]').forEach(button => button.addEventListener("click", () => answerShortQuestion(button.dataset.shortGrade === "mastered")));
  }

  function answerShortQuestion(mastered) {
    if (quiz.locked) return;
    quiz.locked = true;
    const question = quiz.items[quiz.index];
    recordQuizOutcome(question, mastered);
    $$('[data-short-grade]').forEach(button => {
      button.disabled = true;
      if ((button.dataset.shortGrade === "mastered") === mastered) button.classList.add("selected");
    });
    $("#short-answer-status").textContent = mastered ? "已记录为掌握。" : "已加入错题回顾，之后可以重新口述。";
    $("#next-question").textContent = quiz.index === quiz.items.length - 1 ? "查看结果 →" : "下一题 →";
    $("#next-question").classList.remove("hidden");
  }

  function nextQuestion() {
    quiz.index++;
    if (quiz.index >= quiz.items.length) finishQuiz(); else renderQuestion();
  }

  function finishQuiz() {
    $("#quiz-session").classList.add("hidden");
    const percent = quiz.items.length ? Math.round(quiz.correct / quiz.items.length * 100) : 0;
    $("#quiz-result").innerHTML = `<p class="eyebrow">ROUND COMPLETE</p><div class="result-score">${percent}<small>%</small></div><h2>${percent >= 80 ? "掌握得不错" : percent >= 60 ? "再复习一轮会更稳" : "先回看相关章节"}</h2><p>本轮面试练习掌握 ${quiz.correct} / ${quiz.items.length} 题。答错或自评未掌握的题已经加入错题回顾。</p><button class="primary-button" id="restart-quiz">再来一轮</button>`;
    $("#quiz-result").classList.remove("hidden");
    $("#restart-quiz").addEventListener("click", () => startQuiz(false));
  }

  function renderMistakes() {
    const items = state.mistakes.map(id => DATA.questions.find(question => question.id === id)).filter(Boolean);
    if (!items.length) {
      $("#mistake-list").innerHTML = '<div class="empty-state"><strong>错题本是空的</strong>完成面试练习后，做错的题会自动出现在这里。</div>';
      return;
    }
    $("#mistake-list").innerHTML = items.map(question => {
      const answer = isShortAnswerQuestion(question)
        ? `<div class="mistake-answer"><strong>参考要点</strong><ul>${(question.points || []).map(point => `<li>${escapeHtml(point)}</li>`).join("")}</ul></div>${question.sample ? `<p>${escapeHtml(question.sample)}</p>` : ""}`
        : `<div class="mistake-answer"><strong>正确答案</strong> ${String.fromCharCode(65 + question.answer)}. ${escapeHtml(question.options[question.answer])}</div><p>${escapeHtml(question.why)}</p>`;
      return `<article class="mistake-card"><span class="card-tag">${escapeHtml(question.category)} · ${isShortAnswerQuestion(question) ? "简答题" : "选择题"}</span><h3>${escapeHtml(question.q)}</h3>${answer}<button data-mastered="${question.id}">✓ 已理解，移出错题本</button></article>`;
    }).join("");
    $$('[data-mastered]').forEach(button => button.addEventListener("click", () => {
      state.mistakes = state.mistakes.filter(id => id !== button.dataset.mastered);
      saveState(); renderMistakes();
    }));
  }

  function formatNumber(value) {
    return new Intl.NumberFormat("zh-CN").format(Number(value) || 0);
  }

  function projectTechnologyBadge(project, className) {
    const technologyText = [...(project.tags || []), ...(project.scan?.technologies || [])].join(" ");
    if (/C\+\+/i.test(technologyText) && /CUDA/i.test(technologyText)) {
      return `<span class="${className} project-tech-badge"><b>C++</b><small>CUDA</small></span>`;
    }
    return `<span class="${className}">${escapeHtml((project.tags?.[0] || "DEV").slice(0, 4))}</span>`;
  }

  function renderProjectHome() {
    const root = $("#project-home-grid");
    if (!root) return;
    $("#project-home-summary").textContent = projects.length ? `${projects.length} 个本地项目 · 可进行项目拷打` : "等待添加第一个本地项目";
    if (!projects.length) {
      root.innerHTML = `<button class="project-add-card" type="button" data-open-projects>
        <span class="project-add-icon">P+</span>
        <span><strong>添加你的第一个项目</strong><small>配置本地目录后，自动生成项目概览与专属面试追问。</small></span>
        <i>→</i>
      </button>`;
    } else {
      root.innerHTML = projects.slice(0, 3).map((project, index) => `<button class="project-home-card" type="button" data-project-home-index="${index}">
        ${projectTechnologyBadge(project, "project-home-icon")}
        <span class="project-home-copy"><small>${escapeHtml(project.role || "项目实践")}</small><strong>${escapeHtml(project.name)}</strong><em>${escapeHtml(project.summary || "等待补充项目简介")}</em></span>
        <span class="project-home-meta">${formatNumber(project.scan?.sourceFileCount)} 个源码文件 <i>→</i></span>
      </button>`).join("") + (projects.length > 3 ? `<button class="project-more-card" type="button" data-open-projects><strong>+${projects.length - 3}</strong><span>查看全部项目</span></button>` : "");
    }
    $$('[data-open-projects]', root).forEach(button => button.addEventListener("click", () => route("projects")));
    $$('[data-project-home-index]', root).forEach(button => button.addEventListener("click", () => {
      currentProjectIndex = Number(button.dataset.projectHomeIndex) || 0;
      route("projects");
      renderProjects();
    }));
  }

  function selectedProjectAiModel() {
    const saved = localStorage.getItem(PROJECT_AI_MODEL_KEY) || "";
    if (aiStatus.method === "chatgpt") return aiModels.includes(saved) ? saved : aiModels[0] || "";
    return saved || aiStatus.model || aiModels[0] || "";
  }

  function selectedProjectAiEffort() {
    const supported = projectReasoningEfforts(selectedProjectAiModel());
    if (!supported.length) return "";
    const saved = localStorage.getItem(PROJECT_AI_EFFORT_KEY) || "medium";
    if (supported.includes(saved)) return saved;
    return supported.includes("medium") ? "medium" : supported[0];
  }

  function projectReasoningEfforts(model) {
    const modelId = String(model || "").trim().toLowerCase();
    if (/^gpt-6\.1-sol(?:[-.]|$)|^gpt-6-astra(?:[-.]|$)/.test(modelId)) return ["low", "medium", "high", "xhigh", "max"];
    if (/^gpt-6\.1-terra(?:[-.]|$)|^gpt-6-(?:sol|luna)(?:[-.]|$)|^gpt-5\.6(?:[-.]|$)/.test(modelId)) return ["none", "low", "medium", "high", "xhigh", "max"];
    if (/^gpt-5\.5(?:[-.]|$)/.test(modelId)) return ["none", "low", "medium", "high", "xhigh"];
    if (/^gpt-5\.(?:1|2|4)(?:[-.]|$)/.test(modelId)) return ["none", "low", "medium", "high", "xhigh"];
    if (/^gpt-5(?:[-.]|$)/.test(modelId)) return ["minimal", "low", "medium", "high"];
    if (/^o[1-9](?:[-.]|$)/.test(modelId)) return ["low", "medium", "high"];
    return [];
  }

  function renderAiConfigDialog() {
    const panel = $("#ai-account-panel");
    const loginButton = $("#chatgpt-login");
    const addAccountButton = $("#chatgpt-add-account");
    const fallback = $("#ai-api-fallback");
    if (!panel || !loginButton || !addAccountButton || !fallback) return;
    const accounts = Array.isArray(aiStatus.accounts) ? aiStatus.accounts : [];
    const activeAccount = accounts.find(account => account.active) || null;
    const savedAccounts = accounts.length ? `<div class="ai-saved-accounts"><small>已保存的 ChatGPT 账号</small>${accounts.map(account => `<button type="button" data-chatgpt-account="${escapeHtml(account.id)}" ${account.active && account.signedIn ? "disabled" : ""}><span>${escapeHtml((account.name || account.email || "C").slice(0, 1).toUpperCase())}</span><div><strong>${escapeHtml(account.name || "ChatGPT 账号")}</strong><em>${escapeHtml(account.email || (account.signedIn ? "已登录" : "需要重新登录"))}</em></div><b>${account.active && account.signedIn ? "当前" : account.signedIn ? "切换" : "登录"}</b></button>`).join("")}</div>` : "";
    if (aiStatus.signedIn) {
      const accountName = aiStatus.account?.name || "ChatGPT 账号";
      const accountEmail = aiStatus.account?.email || "已验证账号";
      panel.innerHTML = `<div class="ai-current-account"><div class="ai-account-avatar">${escapeHtml(accountName.slice(0, 1).toUpperCase())}</div><div class="ai-account-copy"><small>当前 ChatGPT 账号</small><strong>${escapeHtml(accountName)}</strong><span>${escapeHtml(accountEmail)}</span><em>${aiStatus.planEnabled ? "ChatGPT 套餐调用已授权" : "已登录，但尚未授权套餐调用"}</em></div><div class="ai-account-actions"><a href="https://chatgpt.com/#settings/Usage" target="_blank" rel="noopener noreferrer">查看用量 ↗</a><button id="chatgpt-logout" type="button">退出当前账号</button></div></div>${savedAccounts}`;
      loginButton.textContent = aiStatus.planEnabled ? "重新授权当前账号" : "启用当前账号的套餐调用";
      $("#chatgpt-logout")?.addEventListener("click", logoutChatGpt);
    } else {
      panel.innerHTML = `<div class="ai-account-empty"><span>AI</span><div><strong>${activeAccount ? "当前账号需要重新登录" : "尚未使用 ChatGPT 登录"}</strong><p>登录后可使用符合资格的 ChatGPT 套餐运行项目面试。</p></div></div>${savedAccounts}`;
      loginButton.textContent = activeAccount ? "登录当前 ChatGPT 账号" : "Continue with ChatGPT";
    }
    addAccountButton.classList.toggle("hidden", !accounts.length && !aiStatus.signedIn);
    $$('[data-chatgpt-account]', panel).forEach(button => button.addEventListener("click", () => switchChatGptAccount(button.dataset.chatgptAccount)));
    fallback.classList.toggle("ready", aiStatus.apiKeyFallback);
    fallback.innerHTML = `<strong>OpenAI API Key ${aiStatus.apiKeyFallback ? "· 已配置" : "· 未配置"}</strong><p>${aiStatus.apiKeyFallback ? "ChatGPT 未登录或套餐调用不可用时，会自动使用本机 API Key。" : "如不使用 ChatGPT 登录，可在 ~/.config/xiaochen-study/openai.env 中配置 API Key。"}</p>`;
  }

  function openAiConfigDialog() {
    renderAiConfigDialog();
    $("#ai-config-dialog")?.showModal();
  }

  async function beginChatGptLogin(mode = "current", accountId = "") {
    if (window.location.protocol === "file:") {
      const redirected = await redirectFileModeToLocalServer();
      if (!redirected) window.alert("ChatGPT 登录需要本机网站服务。请双击“启动学习网站.desktop”，再进入项目专区登录。");
      return;
    }
    const query = new URLSearchParams();
    if (mode === "add") query.set("mode", "add");
    if (accountId) query.set("account", accountId);
    const startUrl = localRunnerUrl(`/api/ai/chatgpt/start${query.size ? `?${query}` : ""}`);
    const initialAccounts = Array.isArray(aiStatus.accounts) ? aiStatus.accounts : [];
    const initialActiveId = initialAccounts.find(account => account.active)?.id || "";
    const initialAuthSignal = chatGptAuthSignal;
    const popup = window.open(startUrl, "xiaochen-chatgpt-login", "popup,width=620,height=760");
    const progress = $("#ai-login-progress");
    const button = $("#chatgpt-login");
    const addButton = $("#chatgpt-add-account");
    progress?.classList.remove("hidden");
    if (button) button.disabled = true;
    if (addButton) addButton.disabled = true;
    try {
      if (!popup) throw new Error("浏览器阻止了登录窗口，请允许此网站打开弹窗后重试");
      const startedAt = Date.now();
      while (Date.now() - startedAt < 2 * 60_000) {
        await new Promise(resolve => setTimeout(resolve, 1200));
        const statusResponse = await fetch(localRunnerUrl("/api/ai/status"), {cache:"no-store", headers:{Accept:"application/json"}});
        const status = await statusResponse.json().catch(() => ({}));
        const statusAccounts = Array.isArray(status.accounts) ? status.accounts : [];
        const activeId = statusAccounts.find(account => account.active)?.id || "";
        const callbackCompleted = chatGptAuthSignal !== initialAuthSignal;
        const loginCompleted = status.signedIn && (callbackCompleted || (mode === "add" && (activeId !== initialActiveId || statusAccounts.length > initialAccounts.length)));
        if (loginCompleted) {
          aiStatus = {...aiStatus, ...status, error:""};
          aiModels = [];
          await loadProjectAiStatus();
          renderAiConfigDialog();
          progress?.classList.add("hidden");
          if (button) button.disabled = false;
          if (addButton) addButton.disabled = false;
          return;
        }
        if (popup?.closed) break;
      }
      throw new Error("尚未检测到登录完成；如果刚完成授权，请稍等片刻后重新打开配置。");
    } catch (error) {
      if (popup && !popup.closed) popup.close();
      progress?.classList.add("hidden");
      if (button) button.disabled = false;
      if (addButton) addButton.disabled = false;
      window.alert(error.message);
    }
  }

  async function switchChatGptAccount(accountId) {
    try {
      const response = await fetch(localRunnerUrl("/api/ai/chatgpt/select"), {method:"POST", headers:{"Content-Type":"application/json", Accept:"application/json"}, body:JSON.stringify({accountId})});
      const data = await response.json().catch(() => ({}));
      if (!response.ok) throw new Error(data.error || "切换账号失败");
      aiModels = [];
      await loadProjectAiStatus();
      renderAiConfigDialog();
      if (data.needsLogin) await beginChatGptLogin("current", accountId);
    } catch (error) {
      window.alert(error.message);
    }
  }

  async function logoutChatGpt() {
    if (!window.confirm("确定退出当前 ChatGPT 账号吗？本机 API Key 备用配置不会被删除。")) return;
    try {
      const response = await fetch(localRunnerUrl("/api/ai/chatgpt/logout"), {method:"POST", headers:{Accept:"application/json"}});
      const data = await response.json().catch(() => ({}));
      if (!response.ok) throw new Error(data.error || "退出失败");
      aiModels = [];
      await loadProjectAiStatus();
      renderAiConfigDialog();
      if (!data.revoked) window.alert("本机登录已清除，但远端会话未能确认注销；你可以在 ChatGPT 设置中断开此应用。");
    } catch (error) {
      window.alert(error.message);
    }
  }

  function renderProjectAiHeader() {
    const select = $("#project-ai-model");
    const effortSelect = $("#project-ai-effort");
    const connection = $("#project-ai-connection");
    const badge = $("#project-ai-badge");
    const configButton = $("#project-ai-config");
    if (!select || !effortSelect || !connection || !badge) return;
    const selected = selectedProjectAiModel();
    const selectedEffort = selectedProjectAiEffort();
    const reasoningEfforts = projectReasoningEfforts(selected);
    const reasoningSupported = reasoningEfforts.length > 0;
    const models = [...new Set((aiStatus.method === "chatgpt" ? aiModels : [selected, aiStatus.model, ...aiModels]).filter(Boolean))];
    select.innerHTML = models.length
      ? models.map(model => `<option value="${escapeHtml(model)}" ${model === selected ? "selected" : ""}>${escapeHtml(model)}</option>`).join("")
      : '<option value="">暂无可用模型</option>';
    select.disabled = !aiStatus.configured || !models.length;
    select.onchange = () => {
      if (select.value) localStorage.setItem(PROJECT_AI_MODEL_KEY, select.value);
      aiStatus.error = "";
      renderProjectAiHeader();
      renderProjects();
    };
    effortSelect.innerHTML = reasoningSupported
      ? reasoningEfforts.map(value => `<option value="${value}" ${value === selectedEffort ? "selected" : ""}>${PROJECT_AI_EFFORT_OPTIONS[value]}</option>`).join("")
      : '<option value="">当前模型不支持</option>';
    effortSelect.disabled = !aiStatus.configured || !reasoningSupported;
    effortSelect.onchange = () => {
      localStorage.setItem(PROJECT_AI_EFFORT_KEY, effortSelect.value);
      renderProjectAiHeader();
      renderProjects();
    };
    if (aiStatus.configured) {
      connection.textContent = aiStatus.error ? `模型列表读取失败：${aiStatus.error}` : reasoningSupported ? `${aiStatus.provider} · ${PROJECT_AI_EFFORT_LABELS[selectedEffort]}强度` : `${aiStatus.provider} · 当前模型不支持强度切换`;
      badge.textContent = "● AI 面试已连接";
      badge.classList.add("connected");
      if (configButton) {
        configButton.classList.add("configured");
        configButton.textContent = aiStatus.method === "chatgpt" ? "AI 面试官配置 · ChatGPT" : "AI 面试官配置 · API";
      }
    } else {
      connection.textContent = aiStatus.error || "登录 ChatGPT 后可启用 AI 连续追问";
      badge.textContent = "● 本地题库可用";
      badge.classList.remove("connected");
      if (configButton) {
        configButton.classList.remove("configured");
        configButton.textContent = "AI 面试官配置";
      }
    }
    renderAiConfigDialog();
  }

  async function loadProjectAiStatus() {
    renderProjectAiHeader();
    try {
      const response = await fetch(localRunnerUrl("/api/ai/status"), {cache:"no-store", headers:{Accept:"application/json"}});
      const data = await response.json().catch(() => ({}));
      if (!response.ok) throw new Error(data.error || `AI 状态接口返回 ${response.status}`);
      aiStatus = {...aiStatus, ...data, error:""};
      if (aiStatus.configured) {
        const modelsResponse = await fetch(localRunnerUrl("/api/ai/models"), {cache:"no-store", headers:{Accept:"application/json"}});
        const modelsData = await modelsResponse.json().catch(() => ({}));
        if (Array.isArray(modelsData.models)) aiModels = modelsData.models;
        if (!modelsResponse.ok) aiStatus.error = modelsData.error || "无法读取模型列表";
      }
    } catch (error) {
      aiStatus = {...aiStatus, configured:false, error:`本机 AI 服务未连接：${error.message}`};
    }
    renderProjectAiHeader();
    renderProjects();
    autoGenerateProjectResumes();
  }

  async function loadProjects() {
    renderProjects();
    renderProjectHome();
    try {
      const response = await fetch(localRunnerUrl("/api/projects"), {cache: "no-store", headers: {Accept: "application/json"}});
      const data = await response.json();
      if (!response.ok || !Array.isArray(data.projects)) throw new Error(data.error || "项目数据格式不正确");
      projects = data.projects;
      currentProjectIndex = clamp(currentProjectIndex, 0, Math.max(0, projects.length - 1));
      renderProjectHome();
      renderProjects();
      autoGenerateProjectResumes();
    } catch (error) {
      console.warn("项目专区使用静态数据：", error.message);
    }
  }

  async function generateDetailedProjectResume(project, {force = false} = {}) {
    if (!project?.id || project.resumeGeneration?.source === "manual" || projectResumeGenerationPending.has(project.id)) return;
    if (!aiStatus.configured) return;
    const requestedModel = selectedProjectAiModel();
    const requestedEffort = selectedProjectAiEffort();
    projectResumeGenerationPending.add(project.id);
    const startingIndex = projects.findIndex(item => item.id === project.id);
    if (startingIndex >= 0) {
      projects[startingIndex] = {...projects[startingIndex], resumeGeneration:{...projects[startingIndex].resumeGeneration, pending:true, error:"", requestedModel, requestedEffort}};
      renderProjects();
    }
    try {
      const response = await fetch(localRunnerUrl("/api/projects/resume"), {
        method:"POST",
        headers:{"Content-Type":"application/json", Accept:"application/json"},
        body:JSON.stringify({projectId:project.id, model:requestedModel, reasoningEffort:requestedEffort, force})
      });
      const data = await response.json().catch(() => ({}));
      if (!response.ok || !data.project) throw new Error(data.error || `详细简历接口返回 ${response.status}`);
      const currentIndex = projects.findIndex(item => item.id === project.id);
      if (currentIndex >= 0) projects[currentIndex] = data.project;
    } catch (error) {
      const currentIndex = projects.findIndex(item => item.id === project.id);
      if (currentIndex >= 0) {
        projects[currentIndex] = {
          ...projects[currentIndex],
          resumeGeneration:{...projects[currentIndex].resumeGeneration, pending:false, needsGeneration:true, error:error.message || "详细简历生成失败"}
        };
      }
    } finally {
      projectResumeGenerationPending.delete(project.id);
      const currentIndex = projects.findIndex(item => item.id === project.id);
      if (currentIndex >= 0 && projects[currentIndex].resumeGeneration?.pending) {
        projects[currentIndex] = {...projects[currentIndex], resumeGeneration:{...projects[currentIndex].resumeGeneration, pending:false}};
      }
      renderProjectHome();
      renderProjects();
    }
  }

  async function switchProjectResumeMode(project, mode) {
    const activeMode = project.resumeGeneration?.source === "manual" || project.resume?.mode === "manual" ? "manual" : "auto";
    if (!project?.id || mode === activeMode || !["manual", "auto"].includes(mode) || projectResumeModePending.has(project.id) || projectResumeGenerationPending.has(project.id)) return;
    projectResumeModePending.add(project.id);
    renderProjects();
    try {
      const response = await fetch(localRunnerUrl("/api/projects/resume-mode"), {
        method:"POST",
        headers:{"Content-Type":"application/json", Accept:"application/json"},
        body:JSON.stringify({projectId:project.id, mode})
      });
      const data = await response.json().catch(() => ({}));
      if (!response.ok || !data.project) throw new Error(data.error || `模式切换接口返回 ${response.status}`);
      const currentIndex = projects.findIndex(item => item.id === project.id);
      if (currentIndex >= 0) projects[currentIndex] = data.project;
    } catch (error) {
      window.alert(`简历模式切换失败：${error.message || "未知错误"}`);
    } finally {
      projectResumeModePending.delete(project.id);
      renderProjectHome();
      renderProjects();
    }
    if (mode === "auto") autoGenerateProjectResumes();
  }

  async function markManualResumeReviewed(project) {
    if (!project?.id || projectResumeReviewPending.has(project.id)) return;
    projectResumeReviewPending.add(project.id);
    renderProjects();
    try {
      const response = await fetch(localRunnerUrl("/api/projects/resume-reviewed"), {
        method:"POST",
        headers:{"Content-Type":"application/json", Accept:"application/json"},
        body:JSON.stringify({projectId:project.id})
      });
      const data = await response.json().catch(() => ({}));
      if (!response.ok || !data.project) throw new Error(data.error || `复核状态接口返回 ${response.status}`);
      const currentIndex = projects.findIndex(item => item.id === project.id);
      if (currentIndex >= 0) projects[currentIndex] = data.project;
    } catch (error) {
      window.alert(`手写简历复核状态保存失败：${error.message || "未知错误"}`);
    } finally {
      projectResumeReviewPending.delete(project.id);
      renderProjectHome();
      renderProjects();
    }
  }

  function autoGenerateProjectResumes() {
    if (!aiStatus.configured || projectResumeAutoRun) return projectResumeAutoRun;
    const projectIds = projects
      .filter(project => project.resumeGeneration?.automatic && project.resumeGeneration?.needsGeneration)
      .map(project => project.id);
    if (!projectIds.length) return null;
    projectResumeAutoRun = (async () => {
      for (const projectId of projectIds) {
        const project = projects.find(item => item.id === projectId);
        if (project?.resumeGeneration?.needsGeneration) await generateDetailedProjectResume(project);
      }
    })().finally(() => { projectResumeAutoRun = null; });
    return projectResumeAutoRun;
  }

  function renderProjects() {
    const empty = $("#projects-empty");
    const workspace = $("#projects-workspace");
    if (!empty || !workspace) return;
    const hasProjects = projects.length > 0;
    empty.classList.toggle("hidden", hasProjects);
    workspace.classList.toggle("hidden", !hasProjects);
    if (!hasProjects) return;
    currentProjectIndex = clamp(currentProjectIndex, 0, projects.length - 1);
    $("#project-count").textContent = `${projects.length} 个`;
    $("#project-list").innerHTML = projects.map((project, index) => `<button type="button" class="project-list-item ${index === currentProjectIndex ? "active" : ""}" data-project-index="${index}">
      ${projectTechnologyBadge(project, "project-list-icon")}
      <div><strong>${escapeHtml(project.name)}</strong><small>${escapeHtml(project.role || "待补充职责")}</small></div>
      <i class="${project.available ? "ready" : "unavailable"}" title="${project.available ? "目录读取正常" : escapeHtml(project.error || "目录不可用")}"></i>
    </button>`).join("");
    $$('[data-project-index]', $("#project-list")).forEach(button => button.addEventListener("click", () => {
      currentProjectIndex = Number(button.dataset.projectIndex);
      projectInterview = {projectId:null, mode:"local", items:[], index:0, history:[], currentQuestion:null, sessionId:null, source:null, reasoningEffort:""};
      renderProjects();
    }));
    renderProjectDetail(projects[currentProjectIndex]);
  }

  function conciseResumePitch(resume, fallback) {
    const explicit = String(resume?.pitch || "").trim();
    if (explicit) return explicit;
    const source = String(resume?.overview || fallback || "").trim();
    const sentences = source.match(/[^。！？；]+[。！？；]?/g) || [source];
    let result = "";
    for (const sentence of sentences) {
      if (result && result.length + sentence.length > 260) break;
      result += sentence;
      if (result.length >= 120) break;
    }
    return result || source.slice(0, 260);
  }

  function conciseResumeStack(stack, tags) {
    const items = String(stack || "").split(/[、,，;；]+/).map(item => item.trim()).filter(Boolean);
    return [...new Set(items.length ? items : tags || [])].slice(0, 14);
  }

  function conciseResumeBullets(resume, hasCudaFocus) {
    const explicit = Array.isArray(resume?.resumeBullets) ? resume.resumeBullets.filter(Boolean).slice(0, 6) : [];
    if (explicit.length >= 4) return explicit;
    const source = Array.isArray(resume?.bullets) ? resume.bullets.filter(Boolean) : [];
    const selected = [];
    const take = (pattern, count) => {
      for (const item of source) {
        if (selected.length >= 6 || count <= 0) break;
        if (!selected.includes(item) && pattern.test(item)) {
          selected.push(item);
          count -= 1;
        }
      }
    };
    take(/C\+\+|std::|mutex|condition_variable|线程|并发|队列|对象池|所有权|RAII|资源生命周期|recvmmsg/i, hasCudaFocus ? 2 : 3);
    if (hasCudaFocus) take(/CUDA|GPU|Kernel|cuFFT|显存|Device|Host|Grid|Block|Warp|归约|重采样/i, 3);
    take(/性能|延迟|吞吐|NUMA|亲和|复位|世代|稳定|故障|异常|观测|Nsight/i, 1);
    for (const item of source) {
      if (selected.length >= 6) break;
      if (!selected.includes(item)) selected.push(item);
    }
    return selected;
  }

  function technicalItemParts(value, index, fallbackLabel) {
    const text = String(value || "").trim();
    const separatorIndex = text.search(/[：；，。]/);
    if (separatorIndex > 2 && separatorIndex <= 38) {
      return {title:text.slice(0, separatorIndex), body:text.slice(separatorIndex + 1).trim() || text};
    }
    return {title:`${fallbackLabel} ${String(index + 1).padStart(2, "0")}`, body:text};
  }

  function renderDeepPointList(items, fallbackLabel, className = "") {
    return `<div class="project-deep-points ${className}">${items.map((item, index) => {
      const part = technicalItemParts(item, index, fallbackLabel);
      return `<details class="project-deep-point" ${index < 2 ? "open" : ""}><summary><span>${String(index + 1).padStart(2, "0")}</span><strong>${escapeHtml(part.title)}</strong><i>展开</i></summary><p>${escapeHtml(part.body)}</p></details>`;
    }).join("")}</div>`;
  }

  function projectFlowItems(project, modules) {
    const configured = Array.isArray(project.resume?.flow) ? project.resume.flow.filter(Boolean).slice(0, 8) : [];
    if (configured.length >= 3) return configured;
    const arrowText = [...(project.highlights || []), ...(modules || [])].find(item => String(item).includes("→"));
    if (arrowText) {
      const match = String(arrowText).match(/(?:UDP|输入|接收)[^，。；]*(?:→[^，。；]+){2,}/);
      if (match) return match[0].replace(/数据流.*$/, "").split("→").map(item => item.trim()).filter(Boolean).slice(0, 8);
    }
    return (modules || []).map((item, index) => technicalItemParts(item, index, "阶段").title).slice(0, 6);
  }

  function renderProjectFlow(items) {
    return `<div class="project-system-flow">${items.map((item, index) => {
      const text = String(item || "").trim();
      const separator = text.indexOf("·");
      const lane = separator > 0 ? text.slice(0, separator).trim() : "STAGE";
      const name = separator > 0 ? text.slice(separator + 1).trim() : text;
      return `<div class="project-flow-step"><small>${escapeHtml(lane)}</small><strong>${escapeHtml(name)}</strong><span>${String(index + 1).padStart(2, "0")}</span></div>${index < items.length - 1 ? '<i aria-hidden="true">→</i>' : ""}`;
    }).join("")}</div>`;
  }

  function renderProjectDeepNavigation() {
    const chapters = [
      ["deep-overview", "项目全貌"], ["deep-architecture", "架构数据流"], ["deep-cpp", "C++ 工程"],
      ["deep-cuda", "CUDA 算子"], ["deep-modules", "模块协同"], ["deep-tradeoffs", "难点取舍"],
      ["deep-results", "结果复现"], ["deep-evidence", "证据索引"]
    ];
    return `<nav class="project-deep-navigation" aria-label="技术档案章节">${chapters.map((chapter, index) => `<button type="button" data-deep-target="${chapter[0]}"><span>${String(index + 1).padStart(2, "0")}</span>${chapter[1]}</button>`).join("")}</nav>`;
  }

  function renderProjectResumeDepthSwitcher() {
    return `<div class="project-resume-depth-switcher"><div><strong>内容深度</strong><small>投递版用于直接复制到简历；技术档案用于面试前复习和深挖。</small></div><div role="group" aria-label="简历内容深度"><button type="button" data-project-resume-depth="brief" class="${projectResumeDepth === "brief" ? "active" : ""}">投递精简版</button><button type="button" data-project-resume-depth="deep" class="${projectResumeDepth === "deep" ? "active" : ""}">技术深挖版</button></div></div>`;
  }

  function renderProjectDetail(project) {
    const tags = project.tags?.length ? project.tags : ["待识别技术栈"];
    const highlights = project.highlights?.length ? project.highlights : ["尚未填写项目亮点；添加后可让面试追问更贴近你的真实贡献。"];
    const challenges = project.challenges?.length ? project.challenges : ["尚未填写技术难点；建议补充一次真实的定位、优化或架构取舍。"];
    const categories = [...new Set((project.questions || []).map(question => question.category))];
    const resumeOverview = project.resume?.overview || project.summary || "该项目尚未填写简历描述。";
    const resumeStack = project.resume?.stack || tags.join(" / ");
    const resumeEnvironment = project.resume?.environment?.length ? project.resume.environment : [];
    const resumeArchitecture = project.resume?.architecture || project.presentation?.detailed || "";
    const resumeModules = project.resume?.modules?.length ? project.resume.modules : [];
    const resumeBullets = project.resume?.bullets?.length ? project.resume.bullets : highlights;
    const resumeAlgorithms = project.resume?.algorithms?.length ? project.resume.algorithms : [];
    const resumeEngineering = project.resume?.engineering?.length ? project.resume.engineering : [];
    const resumeChallenges = project.resume?.challenges?.length ? project.resume.challenges : challenges;
    const resumeResults = project.resume?.results?.length ? project.resume.results : [];
    const resumeEvidence = project.resume?.evidence?.length ? project.resume.evidence : [];
    const hasCudaFocus = /(?:CUDA|GPU|Kernel|cuFFT|cuBLAS|CUTLASS|TensorRT)/i.test([resumeStack, ...tags, ...(project.scan?.technologies || [])].join(" "));
    const gpuSectionTitle = hasCudaFocus ? "CUDA Kernel、算子与性能优化" : "核心算法与性能实现";
    const briefPitch = conciseResumePitch(project.resume, project.summary);
    const briefStack = conciseResumeStack(resumeStack, tags);
    const briefBullets = conciseResumeBullets(project.resume, hasCudaFocus);
    const flowItems = projectFlowItems(project, resumeModules);
    const keyFiles = Array.isArray(project.scan?.keyFiles) ? project.scan.keyFiles.slice(0, 8) : [];
    const briefCopy = `${project.name}\n项目角色：${project.role || "核心开发"}${project.period ? `\n项目周期：${project.period}` : ""}\n技术栈：${briefStack.join("、")}\n\n项目概述：\n${briefPitch}\n\n核心贡献：\n${briefBullets.map(item => `• ${item}`).join("\n")}`;
    const deepCopy = `${project.name}\n求职方向：C++ 开发 / CUDA 算子开发\n项目角色：${project.role || "核心开发"}${project.period ? `\n项目周期：${project.period}` : ""}\n项目类型：${project.status || "工程项目"}\n\n一、项目全貌与职责边界\n${resumeOverview}\n\n核心技术栈：\n${resumeStack}\n\n二、系统架构与 CPU/GPU 数据流\n${flowItems.map((item, index) => `${index + 1}. ${item}`).join("\n")}\n\n${resumeArchitecture}${resumeEngineering.length ? `\n\n三、C++ 并发、所有权与资源生命周期\n${resumeEngineering.map((item, index) => `${index + 1}. ${item}`).join("\n")}` : ""}${resumeAlgorithms.length ? `\n\n四、${gpuSectionTitle}\n${resumeAlgorithms.map((item, index) => `${index + 1}. ${item}`).join("\n")}` : ""}${resumeModules.length ? `\n\n五、模块职责与协同边界\n${resumeModules.map((item, index) => `${index + 1}. ${item}`).join("\n")}` : ""}\n\n六、可靠性、故障边界与设计取舍\n${resumeChallenges.map((item, index) => `${index + 1}. ${item}`).join("\n")}\n\n七、工程结果、运行规模与复现条件\n${resumeResults.map(item => `• ${item}`).join("\n")}${resumeEnvironment.length ? `\n\n运行环境：\n${resumeEnvironment.map(item => `• ${item}`).join("\n")}` : ""}${resumeEvidence.length ? `\n\n八、待补性能与正确性证据\n${resumeEvidence.map(item => `• ${item}`).join("\n")}` : ""}${keyFiles.length ? `\n\n源码证据入口：\n${keyFiles.map(file => `• ${file.path}（${file.language}，${file.lines} 行）`).join("\n")}` : ""}`;
    const briefCard = `<article class="project-material-card resume concise-resume"><div class="project-material-title"><span>CV</span><div><small>APPLICATION READY</small><h4>${escapeHtml(project.name)}</h4></div><button type="button" data-project-copy="brief">复制投递版</button></div>
      <div class="project-resume-meta"><span><b>项目角色</b>${escapeHtml(project.role || "核心开发")}</span>${project.period ? `<span><b>项目周期</b>${escapeHtml(project.period)}</span>` : ""}<span><b>目标岗位</b>C++ 开发 / CUDA 算子开发</span></div>
      <div class="project-resume-section resume-brief-pitch"><b>项目概述</b><p>${escapeHtml(briefPitch)}</p></div>
      <div class="project-resume-section"><b>核心技术栈</b><div class="project-resume-stack-chips">${briefStack.map(item => `<span>${escapeHtml(item)}</span>`).join("")}</div></div>
      <div class="project-resume-section resume-brief-contributions"><b>C++ / CUDA 核心贡献</b><ul>${briefBullets.map(item => `<li>${escapeHtml(item)}</li>`).join("")}</ul></div>
      <div class="project-resume-proof"><div><strong>源码与配置可核验</strong><span>当前内容只使用项目中已有的技术证据</span></div><div class="${resumeEvidence.length ? "waiting" : "ready"}"><strong>${resumeEvidence.length ? `${resumeEvidence.length} 项性能证据待补` : "量化证据已补齐"}</strong><span>${resumeEvidence.length ? "未实测数据不会自动写成性能提升" : "可以直接用于投递与面试说明"}</span></div></div>
    </article>`;
    const deepCard = `<article class="project-material-card resume detailed-resume structured-deep-resume"><div class="project-material-title"><span>DB</span><div><small>TECHNICAL DEEP DIVE</small><h4>${escapeHtml(project.name)}</h4></div><button type="button" data-project-copy="deep">复制技术档案</button></div>
      ${renderProjectDeepNavigation()}
      <section class="project-deep-section deep-overview" id="deep-overview">
        <div class="project-deep-heading"><span>01</span><div><small>CONTEXT & OWNERSHIP</small><h5>项目全貌与职责边界</h5></div></div>
        <div class="project-deep-overview-grid"><div><p>${escapeHtml(resumeOverview)}</p><div class="project-resume-stack-chips">${briefStack.map(item => `<span>${escapeHtml(item)}</span>`).join("")}</div></div><aside><span>项目角色</span><strong>${escapeHtml(project.role || "核心开发")}</strong><span>项目类型</span><strong>${escapeHtml(project.status || "工程项目")}</strong>${project.period ? `<span>项目周期</span><strong>${escapeHtml(project.period)}</strong>` : ""}</aside></div>
      </section>
      <section class="project-deep-section" id="deep-architecture">
        <div class="project-deep-heading"><span>02</span><div><small>ARCHITECTURE & DATA FLOW</small><h5>系统架构与 CPU/GPU 数据流</h5></div></div>
        ${renderProjectFlow(flowItems)}
        ${resumeArchitecture ? `<div class="project-architecture-note"><strong>架构说明</strong><p>${escapeHtml(resumeArchitecture)}</p></div>` : ""}
      </section>
      ${resumeEngineering.length ? `<section class="project-deep-section" id="deep-cpp"><div class="project-deep-heading"><span>03</span><div><small>MODERN C++ ENGINEERING</small><h5>C++ 并发、所有权与资源生命周期</h5></div><em>${resumeEngineering.length} 个实现要点</em></div>${renderDeepPointList(resumeEngineering, "C++ 设计", "cpp-points")}</section>` : ""}
      ${resumeAlgorithms.length ? `<section class="project-deep-section" id="deep-cuda"><div class="project-deep-heading"><span>04</span><div><small>CUDA KERNEL & PERFORMANCE</small><h5>${gpuSectionTitle}</h5></div><em>${resumeAlgorithms.length} 个算子要点</em></div>${renderDeepPointList(resumeAlgorithms, "CUDA 设计", "cuda-points")}</section>` : ""}
      ${resumeModules.length ? `<section class="project-deep-section" id="deep-modules"><div class="project-deep-heading"><span>05</span><div><small>MODULE CONTRACTS</small><h5>模块职责与协同边界</h5></div><em>${resumeModules.length} 个核心模块</em></div><div class="project-module-map">${resumeModules.map((item, index) => { const part = technicalItemParts(item, index, "模块"); return `<article><span>${String(index + 1).padStart(2, "0")}</span><h6>${escapeHtml(part.title)}</h6><p>${escapeHtml(part.body)}</p></article>`; }).join("")}</div></section>` : ""}
      <section class="project-deep-section" id="deep-tradeoffs"><div class="project-deep-heading"><span>06</span><div><small>RELIABILITY & TRADE-OFFS</small><h5>可靠性、故障边界与设计取舍</h5></div><em>${resumeChallenges.length} 个关键问题</em></div>${renderDeepPointList(resumeChallenges, "技术取舍", "tradeoff-points")}</section>
      <section class="project-deep-section" id="deep-results"><div class="project-deep-heading"><span>07</span><div><small>RESULTS & REPRODUCTION</small><h5>工程结果、运行规模与复现条件</h5></div></div><div class="project-results-grid"><div><h6>已成立的工程结果</h6><ul>${resumeResults.map(item => `<li>${escapeHtml(item)}</li>`).join("")}</ul></div><div><h6>运行环境与数据规模</h6><ul>${resumeEnvironment.map(item => `<li>${escapeHtml(item)}</li>`).join("")}</ul></div></div></section>
      <section class="project-deep-section" id="deep-evidence"><div class="project-deep-heading"><span>08</span><div><small>EVIDENCE LEDGER</small><h5>量化证据与源码索引</h5></div></div><div class="project-evidence-grid"><div class="project-evidence-pending"><h6>待补性能与正确性证据</h6><p>只有完成实测并保存环境、基线与报告后，才会把对应结论写入投递版。</p><ul>${resumeEvidence.map(item => `<li><span>待验证</span>${escapeHtml(item.replace(/^(?:待实测|待确认)[：:]?\s*/, ""))}</li>`).join("")}</ul></div><div class="project-source-evidence"><h6>源码证据入口</h6><p>按核心文件进入源码核对实现，文件行数会随项目更新自动刷新。</p>${keyFiles.length ? `<ul>${keyFiles.map(file => `<li><code>${escapeHtml(file.path || "")}</code><span>${escapeHtml(file.language || "源码")} · ${Number(file.lines || 0).toLocaleString("zh-CN")} 行</span></li>`).join("")}</ul>` : "<p>当前扫描尚未识别到核心源码文件。</p>"}</div></div></section>
    </article>`;
    $("#project-detail").innerHTML = `
      <section class="project-hero">
        <div class="project-hero-top"><div><p class="eyebrow">${escapeHtml(project.status || "LOCAL PROJECT")}</p><h2>${escapeHtml(project.name)}</h2></div><span class="project-path-state ${project.available ? "ready" : "unavailable"}">${project.available ? "目录已连接" : "目录不可用"}</span></div>
        <p class="project-summary">${escapeHtml(project.summary || "该项目尚未填写简介。")}</p>
        <div class="project-tags">${tags.map(tag => `<span>${escapeHtml(tag)}</span>`).join("")}</div>
        <div class="project-path"><span>本地目录</span><code>${escapeHtml(project.path || "尚未配置")}</code></div>
        ${project.error ? `<div class="project-warning">${escapeHtml(project.error)}</div>` : ""}
      </section>
      <section class="project-presentation-section">
        <div class="project-presentation-heading"><div><p class="eyebrow">RESUME & TECHNICAL DOSSIER · C++ / CUDA</p><h3>简历项目材料</h3></div><p>投递版突出岗位匹配和可验证贡献；技术档案保留架构、实现、取舍与性能证据。</p></div>
        ${renderProjectResumeDepthSwitcher()}
        ${renderProjectResumeAutomation(project)}
        ${projectResumeDepth === "brief" ? briefCard : deepCard}
      </section>
      <section class="project-interview-panel">
        <div class="project-interview-head"><div><p class="eyebrow">AI MOCK INTERVIEW</p><h3>项目连续面试</h3><p>模拟 C++ / CUDA 岗位面试：先用通俗语言确认项目全貌，再重点追问并发、资源管理、Kernel/算子、内存、同步与性能证据。</p></div><span class="project-ai-status ${aiStatus.configured ? "ready" : "offline"}">${aiStatus.configured ? "AI · CONNECTED" : "LOCAL · READY"}</span></div>
        <div class="project-ai-privacy source-submit"><strong>${aiStatus.configured ? "源码分析已开启" : "本地备用模式"}</strong><span>${aiStatus.configured ? `开始 AI 面试时，后台会过滤密钥文件并脱敏常见凭据，再将限量源码摘录和回答发送给 ${escapeHtml(aiStatus.provider)}；账号令牌只保存在本机。` : "当前没有可用的 AI 登录或 API 配置，仍可使用网页内置题库进行本地连续追问。"}</span></div>
        <div class="project-interview-config">
          <label class="select-label">追问方向<select id="project-question-category"><option value="全部">全部</option>${categories.map(category => `<option value="${escapeHtml(category)}">${escapeHtml(category)}</option>`).join("")}</select></label>
          <label class="select-label">拷打强度<select id="project-question-level"><option value="全部">混合难度</option><option value="基础">基础确认</option><option value="深挖">深度追问</option><option value="压力">压力拷打</option></select></label>
          <button class="primary-button compact" id="start-ai-project-interview" type="button" ${aiStatus.configured ? "" : "disabled"}>AI 连续追问</button>
          <button class="secondary-button compact" id="start-project-interview" type="button">本地连续追问</button>
        </div>
        <div id="project-interview-stage" class="project-interview-stage"><div class="project-interview-ready"><span>Q</span><strong>准备好进行一场从项目全貌逐步深入实现的连续面试了吗？</strong><p>前两轮不会直接点名函数、Kernel 或源码文件。${aiStatus.configured ? ` 当前模型：${escapeHtml(selectedProjectAiModel())} · ${PROJECT_AI_EFFORT_LABELS[selectedProjectAiEffort()] ? `${PROJECT_AI_EFFORT_LABELS[selectedProjectAiEffort()]}强度` : "模型默认"}。` : " AI 未连接，可先使用本地连续追问。"}</p></div></div>
      </section>`;
    const copyMaterials = {brief:briefCopy, deep:deepCopy};
    $$('[data-project-copy]', $("#project-detail")).forEach(button => button.addEventListener("click", () => copyCode(copyMaterials[button.dataset.projectCopy] || "", button)));
    $$('[data-project-resume-depth]', $("#project-detail")).forEach(button => button.addEventListener("click", () => {
      projectResumeDepth = button.dataset.projectResumeDepth === "deep" ? "deep" : "brief";
      try { localStorage.setItem(PROJECT_RESUME_DEPTH_KEY, projectResumeDepth); } catch { /* keep the in-memory choice */ }
      renderProjectDetail(projects[currentProjectIndex]);
    }));
    $$('[data-deep-target]', $("#project-detail")).forEach(button => button.addEventListener("click", () => {
      document.getElementById(button.dataset.deepTarget)?.scrollIntoView({behavior:"smooth", block:"start"});
    }));
    $$('[data-project-resume-mode]', $("#project-detail")).forEach(button => button.addEventListener("click", () => switchProjectResumeMode(project, button.dataset.projectResumeMode)));
    $("[data-mark-manual-reviewed]", $("#project-detail"))?.addEventListener("click", () => markManualResumeReviewed(project));
    $("[data-generate-project-resume]", $("#project-detail"))?.addEventListener("click", () => generateDetailedProjectResume(project, {force:true}));
    $("[data-open-project-ai-config]", $("#project-detail"))?.addEventListener("click", openAiConfigDialog);
    $("#start-project-interview").addEventListener("click", () => startProjectInterview(project));
    $("#start-ai-project-interview").addEventListener("click", () => startAiProjectInterview(project));
  }

  function renderProjectResumeAutomation(project) {
    const status = project.resumeGeneration || {};
    const pending = projectResumeGenerationPending.has(project.id) || status.pending;
    const modeChanging = projectResumeModePending.has(project.id);
    const reviewPending = projectResumeReviewPending.has(project.id);
    const activeMode = status.source === "manual" || project.resume?.mode === "manual" ? "manual" : "auto";
    const currentModel = selectedProjectAiModel();
    const currentEffort = selectedProjectAiEffort();
    const settingsMismatch = status.source === "generated" && Boolean(currentModel) && (status.model !== currentModel || status.reasoningEffort !== currentEffort);
    const modeSwitcher = `<div class="project-resume-mode-switcher"><div><strong>简历内容来源</strong><small>${modeChanging ? "正在保存切换…" : "两个版本独立保存，可以随时来回切换。"}</small></div><div role="group" aria-label="简历内容来源"><button type="button" data-project-resume-mode="manual" class="${activeMode === "manual" ? "active" : ""}" ${modeChanging || pending ? "disabled" : ""}>手写版</button><button type="button" data-project-resume-mode="auto" class="${activeMode === "auto" ? "active" : ""}" ${modeChanging || pending ? "disabled" : ""}>ChatGPT 自动版</button></div></div>`;
    if (status.source === "manual") {
      if (status.stale) {
        return `${modeSwitcher}<div class="project-resume-auto-status manual-stale"><span>!</span><div><strong>源码已变化，建议复核手写版</strong><small>检测到项目源码与上次确认时不同。手写内容不会被自动覆盖；检查后点击右侧按钮即可建立新的复核基线。</small></div><button type="button" data-mark-manual-reviewed ${reviewPending ? "disabled" : ""}>${reviewPending ? "正在保存…" : "已复核当前源码"}</button></div>`;
      }
      return `${modeSwitcher}<div class="project-resume-auto-status manual"><span>✓</span><div><strong>当前显示手写详细版</strong><small>切换到 ChatGPT 自动版不会删除这里的内容，之后切回来仍会恢复。</small></div></div>`;
    }
    if (pending) {
      const pendingModel = status.requestedModel || currentModel || "当前模型";
      const pendingEffort = PROJECT_AI_EFFORT_LABELS[status.requestedEffort] || "默认";
      return `${modeSwitcher}<div class="project-resume-auto-status pending"><span></span><div><strong>ChatGPT 正在按 C++ / CUDA 岗位重新分析源码</strong><small>本次固定使用 ${escapeHtml(pendingModel)} · ${escapeHtml(pendingEffort)}强度；生成过程中切换顶部设置只会作用于下一次生成。</small></div></div>`;
    }
    if (status.source === "generated" && !status.needsGeneration) {
      const generatedAt = status.generatedAt ? new Date(status.generatedAt).toLocaleString("zh-CN", {hour12:false}) : "最近一次扫描";
      const effortLabel = PROJECT_AI_EFFORT_LABELS[status.reasoningEffort] ? `${PROJECT_AI_EFFORT_LABELS[status.reasoningEffort]}强度 · ` : "";
      const selectedEffortLabel = PROJECT_AI_EFFORT_LABELS[currentEffort] || "默认";
      return `${modeSwitcher}<div class="project-resume-auto-status generated ${settingsMismatch ? "settings-mismatch" : ""}"><span>AI</span><div><strong>${settingsMismatch ? "当前模型设置与这份简历不同" : "ChatGPT 已按当前源码生成详细版"}</strong><small>${settingsMismatch ? `这份简历由 ${escapeHtml(status.model)} · ${escapeHtml(PROJECT_AI_EFFORT_LABELS[status.reasoningEffort] || status.reasoningEffort || "默认")}强度生成；顶部当前选择为 ${escapeHtml(currentModel)} · ${escapeHtml(selectedEffortLabel)}强度。` : `${escapeHtml(status.model || "当前模型")} · ${effortLabel}${escapeHtml(generatedAt)} · 源码未变化时直接读取本机缓存，不重复调用。`}</small></div><button type="button" data-generate-project-resume>${settingsMismatch ? "用当前设置重新生成" : "重新分析源码"}</button></div>`;
    }
    if (status.error) {
      return `${modeSwitcher}<div class="project-resume-auto-status error"><span>!</span><div><strong>自动生成暂未完成</strong><small>${escapeHtml(String(status.error))}</small></div>${aiStatus.configured ? '<button type="button" data-generate-project-resume>重新生成</button>' : '<button type="button" data-open-project-ai-config>连接 ChatGPT</button>'}</div>`;
    }
    if (status.needsGeneration && aiStatus.configured) {
      return `${modeSwitcher}<div class="project-resume-auto-status waiting"><span>AI</span><div><strong>${status.staleReason || (status.stale ? "检测到源码变化，准备更新详细简历" : "新项目已识别，准备生成详细简历")}</strong><small>系统会自动读取脱敏源码，按 C++ / CUDA 岗位相关度组织内容；以后新增项目也使用相同规则。</small></div><button type="button" data-generate-project-resume>立即生成</button></div>`;
    }
    return `${modeSwitcher}<div class="project-resume-auto-status waiting"><span>AI</span><div><strong>连接 ChatGPT 后自动生成岗位定向简历</strong><small>以后添加任何新项目，都会优先提炼真实存在的 C++ / CUDA 技术，不会让领域业务喧宾夺主。</small></div><button type="button" data-open-project-ai-config>连接 ChatGPT</button></div>`;
  }

  function startProjectInterview(project) {
    const category = $("#project-question-category").value;
    const level = $("#project-question-level").value;
    const questions = project.questions || [];
    const opening = questions.find(question => question.category === "项目介绍" && question.level === "基础" && /1[—-]2 分钟/.test(question.question));
    const architecture = questions.find(question => question.category === "架构与流程" && question.level === "基础");
    const preferred = questions.filter(question => (category === "全部" || question.category === category) && (level === "全部" || question.level === level));
    const middleCategories = ["C++ 工程", "CUDA 算子", "技术选型", "难点与取舍", "性能与结果", "故障与调试"];
    const middle = [...preferred].sort((left, right) => {
      const leftRank = middleCategories.indexOf(left.category);
      const rightRank = middleCategories.indexOf(right.category);
      return (leftRank < 0 ? 99 : leftRank) - (rightRank < 0 ? 99 : rightRank);
    });
    const pool = [...new Map([opening, architecture, ...middle, ...questions].filter(Boolean).map(question => [question.id, question])).values()];
    projectInterview = {projectId:project.id, mode:"local", items:pool, index:0, history:[], currentQuestion:pool[0] || null, sessionId:null, source:null};
    renderProjectInterviewTurn(project, null);
  }

  function renderProjectAiLoading(message) {
    const stage = $("#project-interview-stage");
    if (!stage) return;
    stage.innerHTML = `<div class="project-ai-loading"><span></span><strong>${escapeHtml(message)}</strong><p>首次开始会整理项目资料和脱敏源码摘录，后续追问会沿用同一面试上下文。</p></div>`;
  }

  function renderProjectAiError(project, message) {
    const stage = $("#project-interview-stage");
    if (!stage) return;
    stage.innerHTML = `<div class="project-ai-error"><strong>AI 面试暂时没有完成</strong><p>${escapeHtml(message)}</p><div><button class="primary-button" id="retry-ai-interview" type="button">重新尝试</button><button class="secondary-button" id="fallback-local-interview" type="button">切换本地追问</button></div></div>`;
    $("#retry-ai-interview").addEventListener("click", () => startAiProjectInterview(project));
    $("#fallback-local-interview").addEventListener("click", () => startProjectInterview(project));
  }

  async function requestProjectAiInterview(body) {
    const response = await fetch(localRunnerUrl("/api/projects/interview"), {
      method:"POST",
      headers:{"Content-Type":"application/json", Accept:"application/json"},
      body:JSON.stringify(body)
    });
    const data = await response.json().catch(() => ({}));
    if (!response.ok) throw new Error(data.error || `AI 面试接口返回 ${response.status}`);
    return data;
  }

  async function startAiProjectInterview(project) {
    if (!aiStatus.configured) {
      renderProjectAiError(project, "OpenAI API 尚未连接，请检查本机配置或先使用本地追问。");
      return;
    }
    const category = $("#project-question-category")?.value || "全部";
    const level = $("#project-question-level")?.value || "全部";
    renderProjectAiLoading("正在让 AI 阅读项目结构与源码摘录…");
    try {
      const data = await requestProjectAiInterview({
        action:"start",
        projectId:project.id,
        category,
        level,
        model:selectedProjectAiModel(),
        reasoningEffort:selectedProjectAiEffort()
      });
      const result = data.result || {};
      projectInterview = {
        projectId:project.id,
        mode:"ai",
        items:[],
        index:0,
        history:[],
        sessionId:data.sessionId,
        source:data.source || null,
        reasoningEffort:data.reasoningEffort || selectedProjectAiEffort(),
        currentQuestion:{
          id:"ai-first-question",
          category:result.category || "项目介绍",
          level:result.level || "基础",
          question:result.nextQuestion,
          hint:result.nextHint
        }
      };
      renderProjectInterviewTurn(project, null);
    } catch (error) {
      renderProjectAiError(project, error.message);
    }
  }

  async function submitProjectAiAnswer(project, answer) {
    const current = projectInterview.currentQuestion;
    renderProjectAiLoading("AI 正在评价回答并生成下一轮追问…");
    try {
      const data = await requestProjectAiInterview({
        action:"answer",
        projectId:project.id,
        sessionId:projectInterview.sessionId,
        currentQuestion:current.question,
        answer
      });
      const result = data.result || {};
      projectInterview.history.push({question:current.question, answer, score:Number(result.score) || 0});
      projectInterview.currentQuestion = {
        id:`ai-follow-up-${projectInterview.history.length + 1}`,
        category:result.category || "项目追问",
        level:result.level || "深挖",
        question:result.nextQuestion,
        hint:result.nextHint
      };
      renderProjectInterviewTurn(project, {
        score:Number(result.score) || 0,
        feedback:result.feedback || "AI 已完成本轮评价。",
        strengths:Array.isArray(result.strengths) ? result.strengths : [],
        improvements:Array.isArray(result.improvements) ? result.improvements : []
      });
    } catch (error) {
      renderProjectAiError(project, error.message);
    }
  }

  function analyzeProjectAnswer(project, answer) {
    const checks = [
      {key:"role", pattern:/我(?:主要)?(?:负责|实现|设计|主导|定位|优化)/, strength:"说明了个人职责或具体动作", improvement:"补充你个人主导的模块、决策和交付边界"},
      {key:"flow", pattern:/架构|流程|模块|输入|输出|数据流|队列|流水线/, strength:"覆盖了架构、数据流或模块边界", improvement:"按输入→处理→状态→输出讲清整体链路"},
      {key:"tech", pattern:/CUDA|cuFFT|Kernel|GPU|CPU|UDP|多线程|条件变量|原子|捕获|跟踪|译码|LDPC|RAII/i, strength:"使用了与本项目相关的技术词和原理", improvement:"补充 CUDA、多线程、网络接入或信号处理中的具体实现"},
      {key:"decision", pattern:/因为|选择|相比|取舍|代价|约束|方案|瓶颈|优缺点/, strength:"解释了选型原因或设计取舍", improvement:"说明备选方案、选择理由和接受的代价"},
      {key:"evidence", pattern:/\d|Profil|Nsight|测试|日志|指标|吞吐|延迟|精度|验证|复现|对比/i, strength:"给出了数据、测试或验证线索", improvement:"补充基线、测试环境、指标和验证结果"},
      {key:"risk", pattern:/异常|失败|丢包|复位|失锁|边界|回滚|资源|生命周期|竞态|反压|释放/, strength:"考虑了异常路径、边界或资源风险", improvement:"补充异常路径、边界条件和失败后的恢复方案"}
    ];
    const covered = checks.filter(item => item.pattern.test(answer));
    const missing = checks.filter(item => !item.pattern.test(answer));
    let score = 28 + Math.min(22, Math.round(answer.length / 9)) + covered.length * 8;
    if (answer.length < 60) score = Math.min(score, 48);
    score = clamp(score, 20, 96);
    const strengths = covered.slice(0, 3).map(item => item.strength);
    const improvements = missing.slice(0, 3).map(item => item.improvement);
    const feedback = answer.length < 60
      ? "回答偏短，目前更像结论，还没有形成可以支撑追问的证据链。"
      : `本轮覆盖了 ${covered.length} / ${checks.length} 个关键维度。${missing.length ? "下一问会优先追问你还没说清的部分。" : "基本结构完整，接下来进入具体实现和边界条件。"}`;
    return {score, strengths, improvements, feedback, missingKeys:missing.map(item => item.key)};
  }

  function buildProjectFollowUp(project, answer, analysis) {
    if (projectInterview.history.length < Math.min(5, projectInterview.items.length)) {
      projectInterview.index = Math.min(projectInterview.index + 1, projectInterview.items.length - 1);
      return projectInterview.items[projectInterview.index];
    }
    const targeted = [
      {pattern:/UDP|组播|网络|丢包|解包/i, category:"架构与流程", question:"你提到了 UDP 数据接入。如果出现丢包、乱序或时间基不连续，六级流水线怎样避免继续消费旧数据？"},
      {pattern:/队列|线程|并发|流水线|反压|条件变量/i, category:"并发设计", question:"项目的 safe_queue 使用指针传递数据。请说清所有权转移、队列无界增长的风险，以及下游跟不上时你会怎样做反压。"},
      {pattern:/CUDA|GPU|Kernel|cuFFT|FFT|显存/i, category:"技术选型", question:"你提到了 GPU 加速。请拆分一次捕获计算中的主机与设备传输、cuFFT 计划复用和 Kernel 执行，并说明你用什么证据判断瓶颈在哪里。"},
      {pattern:/捕获|粗捕|细捕|PAR|相关|多普勒/i, category:"信号捕获", question:"项目同时支持独立峰和直接非相干两种粗捕模式。它们的适用场景、PAR 门限取舍，以及连续无峰时的流水线复位策略分别是什么？"},
      {pattern:/跟踪|测距|伪距|失锁|DLL|PLL|FLL|QPSK/i, category:"跟踪与测距", question:"动态测距中，宽环拉入、失锁确认和 DynamicRangeContinuityFilter 各自解决什么问题？错误门限会导致什么假阳性或假阴性？"},
      {pattern:/译码|LDPC|软信息|码字|QPSK/i, category:"译码与数据一致性", question:"跟踪线程交付一个 3 s 周期的软信息后，QC-LDPC 译码如何判断输入仍属于当前 reset_generation？复位恰好发生在译码中途时怎么保证一致性？"},
      {pattern:/RAII|资源|生命周期|释放|所有权/i, category:"资源管理", question:"请选一个同时拥有主机缓冲区和 CUDA 工作区的对象，说明为什么禁止拷贝、析构顺序如何设计，以及异常退出时怎样避免重复释放。"}
    ];
    const matched = targeted.find(item => item.pattern.test(answer));
    if (matched) return {id:`local-follow-up-${projectInterview.history.length + 1}`, level:"深挖", hint:"使用真实数据结构、状态和失败路径回答，不要只重复模块名称。", ...matched};
    if (analysis.missingKeys.includes("evidence")) return {id:"local-evidence", category:"性能与结果", level:"深挖", question:"你刚才给出了方案，但还没有证明它有效。请补充测试环境、输入规模、基线、观测指标和失败标准。", hint:"没有真实数据可以直说尚未量化，但要给出可执行的验证方案。"};
    projectInterview.index = (projectInterview.index + 1) % projectInterview.items.length;
    return projectInterview.items[projectInterview.index];
  }

  function submitProjectInterviewAnswer(project, answer) {
    const current = projectInterview.currentQuestion;
    const analysis = analyzeProjectAnswer(project, answer);
    projectInterview.history.push({question:current.question, answer, score:analysis.score});
    projectInterview.currentQuestion = buildProjectFollowUp(project, answer, analysis);
    renderProjectInterviewTurn(project, analysis);
  }

  function renderProjectInterviewTurn(project, analysis) {
    const stage = $("#project-interview-stage");
    const question = projectInterview.currentQuestion;
    if (!question) {
      stage.innerHTML = '<div class="project-interview-ready"><strong>当前条件下没有可用问题</strong><p>请调整追问方向或拷打强度后重试。</p></div>';
      return;
    }
    const isAiMode = projectInterview.mode === "ai";
    const feedback = analysis ? `<section class="project-ai-feedback">
      <div class="project-ai-score"><strong>${analysis.score}</strong><span>本轮评分</span></div>
      <div><h5>${isAiMode ? "AI 面试官点评" : "本地面试点评"}</h5><p>${escapeHtml(analysis.feedback)}</p><div class="project-ai-review"><span><b>已覆盖要点</b>${analysis.strengths.map(item => `<i>${escapeHtml(item)}</i>`).join("") || "<i>已完成基本作答，下轮需要更具体。</i>"}</span><span><b>还需补充</b>${analysis.improvements.map(item => `<i>${escapeHtml(item)}</i>`).join("") || "<i>结构较完整，继续进入实现细节。</i>"}</span></div></div>
    </section>` : "";
    const round = projectInterview.history.length + 1;
    const sourceSummary = isAiMode && projectInterview.source
      ? ` · 已提交 ${formatNumber(projectInterview.source.fileCount)} 个脱敏源码文件 / ${formatNumber(projectInterview.source.lineCount)} 行`
      : "";
    stage.innerHTML = `${feedback}<article class="project-ai-question">
      <div class="project-question-meta"><span>${escapeHtml(question.category || "项目追问")}</span><b>${escapeHtml(question.level || "深挖")}</b><small>第 ${round} 轮 · ${isAiMode ? `AI 连续面试${sourceSummary}` : "本地连续面试"}</small></div>
      <h4>${escapeHtml(question.question)}</h4>
      <label for="project-local-answer">你的回答</label>
      <textarea id="project-local-answer" rows="7" placeholder="建议先口述，再写下背景、你的动作、技术原理、取舍和验证结果。"></textarea>
      <div id="project-local-hint" class="project-answer-hint hidden"><strong>回答要点</strong><p>${escapeHtml(question.hint || "结合项目中的真实设计、代码和数据回答。")}</p></div>
      <div class="project-ai-answer-footer"><span>${isAiMode ? `本轮回答会通过 ${escapeHtml(aiStatus.provider)}（${PROJECT_AI_EFFORT_LABELS[projectInterview.reasoningEffort] ? `${PROJECT_AI_EFFORT_LABELS[projectInterview.reasoningEffort]}强度` : "模型默认"}）生成点评和下一问。` : "回答只在当前浏览器页面中评价，不会上传。"}</span><div><button class="secondary-button" id="show-local-hint" type="button">查看要点</button><button class="primary-button" id="submit-local-answer" type="button">提交并追问 →</button></div></div>
    </article>`;
    const answerInput = $("#project-local-answer");
    $("#show-local-hint").addEventListener("click", event => {
      $("#project-local-hint").classList.remove("hidden");
      event.currentTarget.disabled = true;
    });
    $("#submit-local-answer").addEventListener("click", () => {
      const answer = answerInput.value.trim();
      if (!answer) {
        answerInput.focus();
        answerInput.classList.add("invalid");
        return;
      }
      if (isAiMode) submitProjectAiAnswer(project, answer);
      else submitProjectInterviewAnswer(project, answer);
    });
    answerInput.addEventListener("input", () => answerInput.classList.remove("invalid"));
  }

  function bindEvents() {
    $$('[data-route]').forEach(button => button.addEventListener("click", () => route(button.dataset.route)));
    $$('[data-open-cuda-interview]').forEach(link => link.addEventListener("click", event => {
      event.preventDefault();
      openCudaInterview();
    }));
    $("#project-ai-config")?.addEventListener("click", openAiConfigDialog);
    $("#chatgpt-login")?.addEventListener("click", () => beginChatGptLogin("current"));
    $("#chatgpt-add-account")?.addEventListener("click", () => beginChatGptLogin("add"));
    window.addEventListener("message", event => {
      const runnerOrigin = window.location.protocol === "file:" ? "http://127.0.0.1:8765" : window.location.origin;
      const allowedAuthOrigins = new Set([runnerOrigin, "http://127.0.0.1:8765"]);
      if (!allowedAuthOrigins.has(event.origin) || event.data?.type !== "xiaochen-chatgpt-auth") return;
      if (event.data.ok) {
        chatGptAuthSignal += 1;
        loadProjectAiStatus();
      }
    });
    $("#back-button").addEventListener("click", () => route(null, {back: true}));
    $("#continue-study").addEventListener("click", () => { route("notes"); selectSection(currentSectionIndex); });
    $("#complete-section").addEventListener("click", toggleComplete);
    $("#prev-section").addEventListener("click", () => selectSection(currentSectionIndex - 1));
    $("#next-section").addEventListener("click", () => selectSection(currentSectionIndex + 1));
    $("#cuda-interview-prev").addEventListener("click", () => {
      selectCudaInterviewArticle(currentCudaInterviewIndex - 1);
      window.scrollTo({top: 0, behavior: "smooth"});
    });
    $("#cuda-interview-next").addEventListener("click", () => {
      selectCudaInterviewArticle(currentCudaInterviewIndex + 1);
      window.scrollTo({top: 0, behavior: "smooth"});
    });
    $("#start-quiz").addEventListener("click", () => startQuiz(false));
    $("#welcome-start").addEventListener("click", () => startQuiz(false));
    $("#next-question").addEventListener("click", nextQuestion);
    $("#clear-mistakes").addEventListener("click", () => {
      if (!state.mistakes.length) return;
      if (!window.confirm("确定清空全部面试错题吗？此操作无法撤销。")) return;
      state.mistakes = [];
      saveState();
      renderMistakes();
    });
  }

  function setupResizableSidebar({layoutSelector, handleSelector, toggleSelector, widthKey, collapsedKey}) {
    const layout = $(layoutSelector);
    const handle = $(handleSelector);
    const toggle = $(toggleSelector);
    let width = Number.parseFloat(localStorage.getItem(widthKey));
    if (!Number.isFinite(width)) width = 282;
    let collapsed = localStorage.getItem(collapsedKey) === "true";

    const setWidth = value => {
      const max = Math.max(240, Math.min(440, window.innerWidth * .42));
      width = clamp(value, 220, max);
      layout.style.setProperty("--notes-sidebar-width", `${Math.round(width)}px`);
      handle.setAttribute("aria-valuenow", String(Math.round(width)));
      handle.setAttribute("aria-valuemin", "220");
      handle.setAttribute("aria-valuemax", String(Math.round(max)));
      return width;
    };
    const setCollapsed = value => {
      collapsed = value;
      layout.classList.toggle("sidebar-collapsed", collapsed);
      toggle.textContent = collapsed ? "›" : "‹";
      const action = collapsed ? "展开章节栏" : "收起章节栏";
      toggle.setAttribute("aria-label", action);
      toggle.title = action;
      toggle.setAttribute("aria-expanded", String(!collapsed));
      localStorage.setItem(collapsedKey, String(collapsed));
    };
    const stopResize = () => document.body.classList.remove("sidebar-resizing");
    handle.addEventListener("pointerdown", event => {
      if (window.innerWidth <= 700 || collapsed || event.button !== 0) return;
      event.preventDefault();
      handle.setPointerCapture(event.pointerId);
      document.body.classList.add("sidebar-resizing");
    });
    handle.addEventListener("pointermove", event => {
      if (!handle.hasPointerCapture(event.pointerId)) return;
      const rect = layout.getBoundingClientRect();
      setWidth(event.clientX - rect.left);
    });
    handle.addEventListener("pointerup", event => {
      if (!handle.hasPointerCapture(event.pointerId)) return;
      handle.releasePointerCapture(event.pointerId);
      localStorage.setItem(widthKey, String(Math.round(width)));
      stopResize();
    });
    handle.addEventListener("pointercancel", stopResize);
    handle.addEventListener("keydown", event => {
      if (event.key !== "ArrowLeft" && event.key !== "ArrowRight") return;
      event.preventDefault();
      setWidth(width + (event.key === "ArrowRight" ? 24 : -24));
      localStorage.setItem(widthKey, String(Math.round(width)));
    });
    handle.addEventListener("dblclick", () => {
      localStorage.removeItem(widthKey);
      setWidth(282);
    });
    toggle.addEventListener("click", () => setCollapsed(!collapsed));
    window.addEventListener("resize", () => setWidth(width));
    setWidth(width);
    setCollapsed(collapsed);
  }

  async function init() {
    if (await redirectFileModeToLocalServer()) return;
    setupCategories();
    setupResizableSidebar({
      layoutSelector: "#notes-view",
      handleSelector: "#notes-sidebar-resizer",
      toggleSelector: "#notes-sidebar-toggle",
      widthKey: SIDEBAR_WIDTH_KEY,
      collapsedKey: SIDEBAR_COLLAPSED_KEY
    });
    setupResizableSidebar({
      layoutSelector: "#cuda-interview-view",
      handleSelector: "#cuda-interview-sidebar-resizer",
      toggleSelector: "#cuda-interview-sidebar-toggle",
      widthKey: CUDA_INTERVIEW_SIDEBAR_WIDTH_KEY,
      collapsedKey: CUDA_INTERVIEW_SIDEBAR_COLLAPSED_KEY
    });
    bindEvents();
    renderMistakes();
    loadNotes();
    loadProjects();
    loadProjectAiStatus();
  }

  init();
})();
