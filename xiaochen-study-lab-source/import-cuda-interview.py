#!/usr/bin/env python3
"""Import browser-saved CUDA articles into the local study knowledge map."""

from __future__ import annotations

import html
import re
from html.parser import HTMLParser
from pathlib import Path


DOWNLOAD_DIR = Path("/home/czq/下载")
PROJECT_DIR = Path(__file__).resolve().parent
OUTPUT_FILE = PROJECT_DIR / "dist" / "cuda-interview.md"


GROUPS = [
    ("1 高频面试题总集", [
        "CUDA 高频面试题 340 问：从硬件数值到 Blackwell 算子优化",
    ]),
    ("2 CUDA 基础与硬件模型", [
        "CUDA 硬件架构：SM、Warp、CUDA Core 与 Tensor Core",
        "CUDA 线程模型：Grid、Block、Thread 与 Global Thread ID",
        "CUDA Warp 机制：分支分化、Active Mask 与 Warp 级原语",
    ]),
    ("3 内存、并发与执行优化", [
        "CUDA Global Memory：合并访问、对齐与带宽优化",
        "CUDA Shared Memory：Bank Conflict、Padding 与 Swizzle",
        "CUDA Registers：寄存器使用、Spilling 与 Local Memory",
        "CUDA Constant Memory 与 Texture Memory：只读数据的访存选择",
        "CUDA Streams：异步执行、Event 同步与计算传输重叠",
        "CUDA 循环展开：#pragma unroll 的原理与取舍",
        "CUDA Kernel Fusion：为什么融合能减少显存带宽瓶颈",
    ]),
    ("4 Profiling 与性能诊断", [
        "CUDA Profiling：Nsight Compute 与 Nsight Systems 常用指标",
        "Nsight Compute 指标全解：从 NCU 报告定位 CUDA Kernel 优化空间",
    ]),
    ("5 手写 CUDA 算子", [
        "手写 CUDA Vector Add 与 Vector Dot Product",
        "手写 CUDA Parallel Reduction：从交错寻址到 Warp 级规约",
        "手写 CUDA Softmax：Online Softmax 与数值稳定性",
        "手写 CUDA LayerNorm / RMSNorm：均值与方差的高效计算",
        "手写 CUDA GEMM：朴素版、Shared Memory Tiling 与 Bank Conflict",
    ]),
    ("6 GEMM、CUTLASS 与进阶优化", [
        "Split-K 详解：从 GEMM K 维并行到 Attention Split-KV",
        "CUTLASS 入门：从 CUDA Tiling 到可组合 GEMM Kernel",
        "CuTe DSL 入门：从 Layout、Tensor 到 GPU GEMM",
        "LMDeploy Grouped GEMM 源码详解：从 C++ 调度到 CUTLASS WGMMA",
    ]),
]


def normalize(value: str) -> str:
    return re.sub(r"\s+", " ", html.unescape(value)).strip()


class ProseToMarkdown(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.in_prose = False
        self.div_depth = 0
        self.skip_depth = 0
        self.in_pre = False
        self.pre_language = "text"
        self.pre_text: list[str] = []
        self.active_tag = ""
        self.active_prefix = ""
        self.current: list[str] = []
        self.link_stack: list[str] = []
        self.list_stack: list[str] = []
        self.blocks: list[str] = []
        self.in_table = False
        self.table_rows: list[list[str]] = []
        self.table_row: list[str] = []
        self.table_cell: list[str] | None = None

    def add_block(self, value: str) -> None:
        value = value.strip()
        if value and (not self.blocks or self.blocks[-1] != value):
            self.blocks.append(value)

    def start_block(self, tag: str, prefix: str = "") -> None:
        if self.active_tag:
            return
        self.active_tag = tag
        self.active_prefix = prefix
        self.current = []

    def finish_block(self, tag: str) -> None:
        if self.active_tag != tag:
            return
        value = normalize("".join(self.current))
        if value:
            self.add_block(f"{self.active_prefix}{value}")
        self.active_tag = ""
        self.active_prefix = ""
        self.current = []

    def append(self, value: str) -> None:
        if self.table_cell is not None:
            self.table_cell.append(value)
        elif self.active_tag:
            self.current.append(value)

    def finish_table(self) -> None:
        rows = [row for row in self.table_rows if any(cell for cell in row)]
        if rows:
            width = max(len(row) for row in rows)
            rows = [row + [""] * (width - len(row)) for row in rows]

            def render(row: list[str]) -> str:
                return "| " + " | ".join(cell.replace("|", "\\|") for cell in row) + " |"

            lines = [render(rows[0]), "| " + " | ".join(["---"] * width) + " |"]
            lines.extend(render(row) for row in rows[1:])
            self.add_block("\n".join(lines))
        self.in_table = False
        self.table_rows = []
        self.table_row = []
        self.table_cell = None

    def handle_starttag(self, tag: str, attrs_list: list[tuple[str, str | None]]) -> None:
        attrs = {key: value or "" for key, value in attrs_list}
        classes = set(attrs.get("class", "").split())

        if not self.in_prose:
            if tag == "div" and "prose" in classes:
                self.in_prose = True
                self.div_depth = 1
            return

        if self.skip_depth:
            if tag not in {"br", "img", "hr", "meta", "link", "input"}:
                self.skip_depth += 1
            return
        if tag in {"button", "script", "style", "svg"}:
            self.skip_depth = 1
            return

        if self.in_pre:
            if tag == "br":
                self.pre_text.append("\n")
            return

        if tag == "div":
            self.div_depth += 1
            return
        if tag == "pre":
            if self.active_tag:
                self.finish_block(self.active_tag)
            self.in_pre = True
            self.pre_language = attrs.get("data-language", "text") or "text"
            self.pre_text = []
            return

        if tag == "table":
            self.in_table = True
            self.table_rows = []
            return
        if self.in_table:
            if tag == "tr":
                self.table_row = []
            elif tag in {"th", "td"}:
                self.table_cell = []
            elif tag == "code" and self.table_cell is not None:
                self.table_cell.append("`")
            return

        if tag in {"ul", "ol"}:
            self.list_stack.append(tag)
            return
        if tag == "li":
            prefix = "1. " if self.list_stack and self.list_stack[-1] == "ol" else "- "
            self.start_block(tag, prefix)
            return
        if tag in {"h2", "h3", "h4", "h5", "h6"}:
            level = min(6, int(tag[1]) + 2)
            self.start_block(tag, "#" * level + " ")
            return
        if tag == "p":
            self.start_block(tag)
            return
        if tag == "blockquote":
            self.start_block(tag, "> ")
            return
        if tag == "summary":
            self.start_block(tag, "**")
            return
        if tag == "br":
            self.append("\n")
            return
        if tag == "hr":
            self.add_block("---")
            return
        if tag == "code":
            self.append("`")
        elif tag in {"strong", "b"}:
            self.append("**")
        elif tag in {"em", "i"}:
            self.append("*")
        elif tag == "a":
            self.append("[")
            self.link_stack.append(attrs.get("href", ""))

    def handle_endtag(self, tag: str) -> None:
        if not self.in_prose:
            return
        if self.skip_depth:
            self.skip_depth -= 1
            return
        if self.in_pre:
            if tag == "pre":
                code = "".join(self.pre_text).replace("\r", "").strip("\n")
                self.add_block(f"```{self.pre_language}\n{code}\n```")
                self.in_pre = False
                self.pre_text = []
            return

        if self.in_table:
            if tag in {"th", "td"} and self.table_cell is not None:
                self.table_row.append(normalize("".join(self.table_cell)))
                self.table_cell = None
            elif tag == "tr" and self.table_row:
                self.table_rows.append(self.table_row)
                self.table_row = []
            elif tag == "table":
                self.finish_table()
            elif tag == "code" and self.table_cell is not None:
                self.table_cell.append("`")
            return

        if tag == "div":
            self.div_depth -= 1
            if self.div_depth <= 0:
                self.in_prose = False
            return
        if tag in {"ul", "ol"}:
            if self.list_stack:
                self.list_stack.pop()
            return
        if tag == "code":
            self.append("`")
        elif tag in {"strong", "b"}:
            self.append("**")
        elif tag in {"em", "i"}:
            self.append("*")
        elif tag == "a":
            href = self.link_stack.pop() if self.link_stack else ""
            self.append(f"]({href})" if href else "]")
        elif tag == "summary" and self.active_tag == tag:
            self.current.append("**")
            self.finish_block(tag)
        elif tag == self.active_tag:
            self.finish_block(tag)

    def handle_data(self, data: str) -> None:
        if not self.in_prose or self.skip_depth:
            return
        if self.in_pre:
            self.pre_text.append(data)
        else:
            self.append(data)

    def markdown(self) -> str:
        value = "\n\n".join(self.blocks)
        value = re.sub(r"(?ms)^#### 目录\s*\n.*?(?=^#### |\Z)", "", value)
        value = value.replace("复制代码", "")
        return re.sub(r"\n{3,}", "\n\n", value).strip()


def extract_meta(source: str) -> tuple[str, str, str]:
    title_match = re.search(r"<title>(.*?)</title>", source, re.S | re.I)
    description_match = re.search(
        r'<meta\s+name="description"\s+content="([^"]*)"', source, re.I
    )
    url_match = re.search(
        r'<meta\s+property="og:url"\s+content="([^"]*)"', source, re.I
    )
    if not title_match:
        raise ValueError("页面缺少标题")
    title = normalize(title_match.group(1)).split(" | ", 1)[0]
    description = normalize(description_match.group(1)) if description_match else ""
    source_url = html.unescape(url_match.group(1)) if url_match else ""
    return title, description, source_url


def import_article(path: Path) -> dict[str, str]:
    source = path.read_text(encoding="utf-8", errors="replace")
    title, description, source_url = extract_meta(source)
    parser = ProseToMarkdown()
    parser.feed(source)
    body = parser.markdown()
    if not body:
        raise ValueError(f"未找到正文：{path.name}")
    return {
        "title": title,
        "description": description,
        "url": source_url,
        "body": body,
    }


def main() -> None:
    pages = sorted(DOWNLOAD_DIR.glob("* _ 霍墨墨 _ AI Infra Notes.html"))
    if not pages:
        raise SystemExit("没有找到已保存的 CUDA 文章。")

    articles = {}
    for path in pages:
        article = import_article(path)
        articles[article["title"]] = article

    expected = [title for _, titles in GROUPS for title in titles]
    missing = [title for title in expected if title not in articles]
    extra = [title for title in articles if title not in expected]
    if missing:
        raise SystemExit("缺少文章：\n- " + "\n- ".join(missing))

    lines = [
        "# CUDA 面试篇",
        "",
        "> **本地学习整理：**内容来自 [霍墨墨 · AI Infra Notes](https://hd9568.github.io/topics/cuda/)。原文著作权归原作者所有，本专题保留每篇文章的原始链接，仅供个人学习复习。",
        "",
        "围绕 CUDA 面试与 GPU 算子优化整理，覆盖基础模型、内存体系、并发执行、性能诊断、手写算子以及 CUTLASS/CuTe 等进阶主题。",
    ]

    for group_title, titles in GROUPS:
        lines.extend(["", f"## {group_title}"])
        for title in titles:
            article = articles[title]
            lines.extend([
                "",
                f"### {title}",
                "",
                f"> 原文：[{title}]({article['url']})　作者：霍墨墨　来源：AI Infra Notes",
            ])
            if article["description"]:
                lines.extend(["", f"> **内容提要：**{article['description']}"])
            lines.extend(["", article["body"]])

    OUTPUT_FILE.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")
    print(f"已导入 {len(expected)} 篇 CUDA 文章，整理为 {len(GROUPS)} 个主题。")
    if extra:
        print("发现但未导入的其他页面：")
        for title in extra:
            print(f"- {title}")


if __name__ == "__main__":
    main()
