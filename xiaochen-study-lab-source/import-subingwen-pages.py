#!/usr/bin/env python3
"""Import browser-saved subingwen.cn Linux articles into the local study site.

Only the #article-container body is converted. Navigation, comments, music,
analytics and other page chrome are intentionally ignored.
"""

from __future__ import annotations

import html
import re
import shutil
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote


DOWNLOAD_DIR = Path("/home/czq/下载")
PROJECT_DIR = Path(__file__).resolve().parent
OUTPUT_FILE = PROJECT_DIR / "dist" / "aux-notes.md"
ASSET_DIR = PROJECT_DIR / "dist" / "assets" / "linux-course"


COURSE = [
    ("第1章 Linux 基础", [
        ("初识Linux操作系统", "初识Linux操作系统"),
        ("文件管理命令", "文件管理命令"),
        ("用户管理命令", "用户管理命令"),
        ("压缩命令", "压缩命令"),
        ("查找命令", "查找命令"),
        ("Vim 的使用", "Vim 的使用"),
        ("GCC", "GCC"),
        ("Linux 静态库和动态库", "静态库和动态库"),
        ("Makefile", "Makefile"),
        ("GDB调试", "GDB调试"),
    ]),
    ("第2章 文件 I/O", [
        ("文件描述符", "文件描述符"),
        ("Linux系统文件IO", "Linux系统 I/O"),
        ("文件的属性信息", "文件状态和属性信息"),
        ("文件描述符复制和重定向", "文件描述符复制和重定向"),
        ("目录遍历", "目录遍历"),
    ]),
    ("第3章 进程和线程", [
        ("进程控制", "进程控制"),
        ("管道", "管道"),
        ("内存映射(mmap)", "内存映射区 mmap"),
        ("共享内存", "共享内存"),
        ("信号", "信号"),
        ("守护进程", "守护进程"),
        ("线程", "多线程"),
        ("线程同步", "线程同步"),
        ("手写线程池 - C语言版", "线程池（C语言版）"),
        ("手写线程池 - C改C++版", "线程池（C++版）"),
    ]),
    ("第4章 套接字通信", [
        ("套接字-Socket", "套接字 Socket"),
        ("三次握手、四次挥手", "三次握手、四次挥手"),
        ("TCP状态转换", "TCP 状态转换"),
        ("服务器并发", "服务器并发"),
        ("TCP数据粘包的处理", "TCP 数据粘包的处理"),
        ("套接字通信类的封装", "套接字通信类的封装"),
        ("IO多路转接（复用）之select", "I/O 多路复用之 select"),
        ("IO多路转接（复用）之poll", "I/O 多路复用之 poll"),
        ("IO多路转接（复用）之epoll", "I/O 多路复用之 epoll"),
        ("基于UDP的套接字通信", "基于 UDP 的套接字通信"),
        ("UDP特性之广播", "UDP 广播"),
        ("UDP特性之组播（多播）", "UDP 组播（多播）"),
    ]),
]


def clean_space(value: str) -> str:
    return re.sub(r"\s+", " ", value).strip()


def safe_asset_name(value: str) -> str:
    value = unquote(value).replace(" ", "-")
    value = re.sub(r"[^0-9A-Za-z._-]+", "-", value)
    return value.strip("-") or "image"


def clean_page_artifacts(value: str) -> str:
    """Remove code-widget labels that browser Save Page injected into prose."""
    if value.startswith("```"):
        return value
    value = re.sub(r"\s+(?:shell|bash|c|cpp)\d+(?=\s*`|\s*$)", "", value)
    value = re.sub(r"^1\. 1 torvalds-says-linux Torvalds$", "", value)
    return value.strip()


class ArticleParser(HTMLParser):
    def __init__(self, html_path: Path, lesson_key: str):
        super().__init__(convert_charrefs=True)
        self.html_path = html_path
        self.lesson_key = lesson_key
        self.resource_dir = html_path.with_name(html_path.stem + "_files")
        self.blocks: list[str] = []
        self.source_url = ""
        self.in_article = False
        self.skip_depth = 0
        self.active_tag = ""
        self.active_prefix = ""
        self.current: list[str] = []
        self.list_stack: list[str] = []
        self.link_stack: list[str | None] = []
        self.in_figure = False
        self.code_language = "text"
        self.in_code_cell = False
        self.code: list[str] = []
        self.in_pre = False
        self.pre_code: list[str] = []
        self.in_table = False
        self.table_rows: list[list[str]] = []
        self.table_row: list[str] = []
        self.table_cell: list[str] | None = None
        self.image_index = 0
        self.copied_images: list[Path] = []
        self.remote_images: list[str] = []
        self.missing_images: list[str] = []

    def add_block(self, value: str) -> None:
        value = value.strip()
        if value and (not self.blocks or self.blocks[-1] != value):
            self.blocks.append(value)

    def start_text_block(self, tag: str, prefix: str = "") -> None:
        if self.active_tag:
            return
        self.active_tag = tag
        self.active_prefix = prefix
        self.current = []

    def finish_text_block(self, tag: str) -> None:
        if self.active_tag != tag:
            return
        value = clean_space("".join(self.current))
        if value:
            self.add_block(self.active_prefix + value)
        self.active_tag = ""
        self.active_prefix = ""
        self.current = []

    def append_text(self, value: str) -> None:
        if self.table_cell is not None:
            self.table_cell.append(value)
        elif self.in_code_cell:
            self.code.append(value)
        elif self.in_pre:
            self.pre_code.append(value)
        elif self.active_tag:
            self.current.append(value)

    def resolve_image(self, attrs: dict[str, str]) -> str | None:
        candidates = [attrs.get("src", ""), attrs.get("data-lazy-src", "")]
        local_path: Path | None = None
        original = ""
        for candidate in candidates:
            if not candidate or candidate.startswith("data:"):
                continue
            original = candidate
            decoded = unquote(candidate.split("?", 1)[0])
            if decoded.startswith(("http://", "https://")):
                basename = Path(decoded).name
                possible = self.resource_dir / basename
            elif decoded.startswith("/"):
                possible = self.resource_dir / Path(decoded).name
            else:
                possible = (self.html_path.parent / decoded).resolve()
            if possible.is_file():
                local_path = possible
                break
            basename = Path(decoded).name
            if self.resource_dir.is_dir():
                matches = list(self.resource_dir.glob(basename))
                if matches:
                    local_path = matches[0]
                    break

        if local_path is None:
            if original.startswith("/"):
                remote = "https://subingwen.cn" + original
                self.remote_images.append(remote)
                return remote
            if original.startswith(("http://", "https://")):
                self.remote_images.append(original)
                return original
            if original:
                self.missing_images.append(original)
            return None

        self.image_index += 1
        target_name = f"{self.lesson_key}-{self.image_index:02d}-{safe_asset_name(local_path.name)}"
        target = ASSET_DIR / target_name
        ASSET_DIR.mkdir(parents=True, exist_ok=True)
        shutil.copy2(local_path, target)
        self.copied_images.append(target)
        return f"assets/linux-course/{target_name}"

    def flush_figure(self) -> None:
        value = "".join(self.code).replace("\r", "").strip("\n")
        value = re.sub(r"\n{3,}", "\n\n", value)
        if value:
            self.add_block(f"```{self.code_language}\n{value}\n```")
        self.in_figure = False
        self.in_code_cell = False
        self.code = []

    def flush_table(self) -> None:
        rows = [row for row in self.table_rows if any(cell.strip() for cell in row)]
        if rows:
            width = max(len(row) for row in rows)
            normalized = [row + [""] * (width - len(row)) for row in rows]
            def row_text(row: list[str]) -> str:
                cells = [clean_space(cell).replace("|", "\\|") for cell in row]
                return "| " + " | ".join(cells) + " |"
            lines = [row_text(normalized[0]), "| " + " | ".join(["---"] * width) + " |"]
            lines.extend(row_text(row) for row in normalized[1:])
            self.add_block("\n".join(lines))
        self.in_table = False
        self.table_rows = []
        self.table_row = []
        self.table_cell = None

    def handle_starttag(self, tag: str, attrs_list: list[tuple[str, str | None]]) -> None:
        attrs = {key: value or "" for key, value in attrs_list}
        classes = set(attrs.get("class", "").split())

        if tag == "article" and attrs.get("id") == "article-container":
            self.in_article = True
            self.source_url = attrs.get("itemtype", "")
            return
        if not self.in_article:
            return

        if self.skip_depth:
            self.skip_depth += 1
            return
        if tag in {"header", "script", "style", "noscript"}:
            self.skip_depth = 1
            return

        if tag == "figure" and "highlight" in classes:
            self.in_figure = True
            language = next((item for item in classes if item not in {"highlight"}), "text")
            self.code_language = {"plaintext": "text", "shell": "bash"}.get(language, language)
            self.code = []
            return
        if self.in_figure:
            if tag == "td" and "code" in classes:
                self.in_code_cell = True
            elif tag == "br" and self.in_code_cell:
                self.code.append("\n")
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
            elif tag == "br" and self.table_cell is not None:
                self.table_cell.append(" ")
            return

        if tag == "pre":
            self.in_pre = True
            self.pre_code = []
            return
        if self.in_pre:
            if tag == "br":
                self.pre_code.append("\n")
            return

        if tag in {"h1", "h2", "h3", "h4", "h5", "h6"}:
            level = min(6, int(tag[1]) + 3)
            self.start_text_block(tag, "#" * level + " ")
        elif tag == "p":
            self.start_text_block(tag)
        elif tag in {"ul", "ol"}:
            self.list_stack.append("-" if tag == "ul" else "1.")
        elif tag == "li":
            marker = self.list_stack[-1] if self.list_stack else "-"
            indent = "  " * max(0, len(self.list_stack) - 1)
            self.start_text_block(tag, f"{indent}{marker} ")
        elif tag == "blockquote":
            self.start_text_block(tag, "> ")
        elif tag == "br":
            self.append_text(" ")
        elif tag == "strong" or tag == "b":
            self.append_text("**")
        elif tag == "em" or tag == "i":
            self.append_text("*")
        elif tag == "code":
            self.append_text("`")
        elif tag == "a":
            skip = "headerlink" in classes or not attrs.get("href")
            self.link_stack.append(None if skip else attrs.get("href"))
            if not skip:
                self.append_text("[")
        elif tag == "img":
            target = self.resolve_image(attrs)
            if target:
                alt = attrs.get("alt") or "课程图片"
                image_md = f"![{alt}]({target})"
                if self.active_tag:
                    self.current.append(image_md)
                else:
                    self.add_block(image_md)

    def handle_endtag(self, tag: str) -> None:
        if not self.in_article:
            return
        if self.skip_depth:
            self.skip_depth -= 1
            return
        if tag == "article":
            self.in_article = False
            return

        if self.in_figure:
            if tag == "td" and self.in_code_cell:
                self.in_code_cell = False
            elif tag == "figure":
                self.flush_figure()
            return

        if self.in_table:
            if tag in {"th", "td"} and self.table_cell is not None:
                self.table_row.append("".join(self.table_cell))
                self.table_cell = None
            elif tag == "tr" and self.table_row:
                self.table_rows.append(self.table_row)
                self.table_row = []
            elif tag == "table":
                self.flush_table()
            return

        if self.in_pre:
            if tag == "pre":
                value = "".join(self.pre_code).strip("\n")
                if value:
                    self.add_block(f"```text\n{value}\n```")
                self.in_pre = False
                self.pre_code = []
            return

        if tag in {"h1", "h2", "h3", "h4", "h5", "h6", "p", "li", "blockquote"}:
            self.finish_text_block(tag)
        elif tag in {"ul", "ol"} and self.list_stack:
            self.list_stack.pop()
        elif tag in {"strong", "b"}:
            self.append_text("**")
        elif tag in {"em", "i"}:
            self.append_text("*")
        elif tag == "code":
            self.append_text("`")
        elif tag == "a" and self.link_stack:
            href = self.link_stack.pop()
            if href:
                self.append_text(f"]({href})")

    def handle_data(self, data: str) -> None:
        if self.in_article and not self.skip_depth:
            self.append_text(data)


def article_path(saved_title: str) -> Path:
    path = DOWNLOAD_DIR / f"{saved_title} _ 爱编程的大丙.html"
    if not path.is_file():
        raise FileNotFoundError(path)
    return path


def main() -> None:
    expected = sum(len(lessons) for _, lessons in COURSE)
    found = list(DOWNLOAD_DIR.glob("* _ 爱编程的大丙.html"))
    if len(found) != expected:
        raise RuntimeError(f"应有 {expected} 篇课程页面，实际找到 {len(found)} 篇")

    output = [
        "# Linux 教程",
        "",
        "> **原文作者：**苏丙榅　｜　**来源：**[爱编程的大丙](https://subingwen.cn/linux/)　｜　**许可协议：**[CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/deed.zh-hans)",
        ">",
        "> 以下内容由我保存的37篇课程网页整理导入，仅用于本地非商业学习。页面结构和资源路径经过适配，正文、代码与课程图片保留原作者署名；如有改动，改动内容继续采用相同许可协议。",
        "",
    ]
    total_images = 0
    total_remote_images = 0
    missing_images: list[tuple[str, str]] = []
    existing_notes = (PROJECT_DIR / "dist" / "aux-notes.md").read_text(encoding="utf-8")
    tcp_fallback_match = re.search(
        r"^### 4\.3 .*?\n(.*?)(?=^### 4\.4 )",
        existing_notes,
        flags=re.MULTILINE | re.DOTALL,
    )
    tcp_fallback = tcp_fallback_match.group(1).strip() if tcp_fallback_match else ""
    tcp_fallback = re.sub(r"^> 原文：.*?\n+", "", tcp_fallback, count=1)

    for chapter_index, (chapter, lessons) in enumerate(COURSE, start=1):
        output.extend([f"## {chapter}", ""])
        for lesson_index, (saved_title, display_title) in enumerate(lessons, start=1):
            path = article_path(saved_title)
            parser = ArticleParser(path, f"{chapter_index:02d}-{lesson_index:02d}")
            parser.feed(path.read_text(encoding="utf-8", errors="replace"))
            parser.close()
            if not parser.blocks:
                if saved_title == "TCP状态转换" and tcp_fallback:
                    parser.blocks = [tcp_fallback]
                    parser.source_url = "https://subingwen.cn/linux/tcp-status/"
                    print("提示：TCP状态转换保存成了目录页，本节暂用现有详细笔记")
                else:
                    raise RuntimeError(f"未能提取正文：{path}")
            source_url = parser.source_url or "https://subingwen.cn/linux/"
            cleaned_blocks = [clean_page_artifacts(block) for block in parser.blocks]
            cleaned_blocks = [block for block in cleaned_blocks if block]
            output.extend([
                f"### {chapter_index}.{lesson_index} {display_title}",
                "",
                f"> 原文：[《{saved_title}》]({source_url})　作者：苏丙榅　来源：爱编程的大丙",
                "",
                *cleaned_blocks,
                "",
            ])
            total_images += len(parser.copied_images)
            total_remote_images += len(parser.remote_images)
            missing_images.extend((saved_title, item) for item in parser.missing_images)

    OUTPUT_FILE.write_text("\n".join(output).rstrip() + "\n", encoding="utf-8")
    print(f"已提取 {expected} 篇文章")
    print(f"已复制 {total_images} 张正文图片")
    print(f"保留 {total_remote_images} 张原站在线图片")
    print(f"输出文件：{OUTPUT_FILE}")
    if missing_images:
        print(f"未找到本地文件的图片引用：{len(missing_images)} 个")
        for title, item in missing_images[:20]:
            print(f"  - {title}: {item}")


if __name__ == "__main__":
    main()
