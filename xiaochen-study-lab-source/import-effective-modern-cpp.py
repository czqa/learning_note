#!/usr/bin/env python3
"""按原目录整理 Effective Modern C++ 的代码与配套图片。"""

from __future__ import annotations

import re
import shutil
from pathlib import Path


PROJECT_DIR = Path(__file__).resolve().parent
SOURCE_DIR = Path("/home/czq/桌面/学习总笔记/辅助学习资料/C++资料/EffectiveModernCppChinese-master")
OUTPUT_FILE = PROJECT_DIR / "dist" / "modern-cpp.md"
ASSET_DIR = PROJECT_DIR / "dist" / "assets" / "modern-cpp"

CHAPTERS = [
    ("第1章 类型推导", "1.DeducingTypes"),
    ("第2章 auto", "2.Auto"),
    ("第3章 移步现代 C++", "3.MovingToModernCpp"),
    ("第4章 智能指针", "4.SmartPointers"),
    ("第5章 右值引用、移动语义与完美转发", "5.RRefMovSemPerfForw"),
    ("第6章 Lambda 表达式", "6.LambdaExpressions"),
    ("第8章 微调", "8.Tweaks"),
]

def natural_key(path: Path) -> list[int | str]:
    return [int(part) if part.isdigit() else part.lower() for part in re.split(r"(\d+)", path.name)]


def copy_local_image(source_image: Path) -> str:
    source_image = source_image.resolve()
    if not source_image.is_file():
        raise FileNotFoundError(f"图片不存在：{source_image}")

    relative_source = source_image.relative_to(SOURCE_DIR)
    safe_prefix = re.sub(r"[^A-Za-z0-9_-]+", "-", relative_source.parent.as_posix()).strip("-")
    destination_name = f"{safe_prefix}-{source_image.name}" if safe_prefix else source_image.name
    destination = ASSET_DIR / destination_name
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source_image, destination)
    return f"assets/modern-cpp/{destination_name}"


def code_block(source_file: Path, heading_level: int = 4) -> str:
    language = "cpp" if source_file.suffix.lower() in {".cpp", ".h", ".hpp"} else "cmake"
    content = source_file.read_text(encoding="utf-8", errors="replace").rstrip()
    return "\n".join([
        f"{'#' * heading_level} `{source_file.name}`",
        f"```{language}",
        content,
        "```",
    ])


def image_block(source_image: Path, heading_level: int = 5) -> str:
    image_path = copy_local_image(source_image)
    return "\n".join([
        f"{'#' * heading_level} 图解：`{source_image.name}`",
        f"![{source_image.name}]({image_path})",
    ])


def item_number(path: Path) -> int:
    match = re.match(r"item(\d+)", path.name, flags=re.IGNORECASE)
    return int(match.group(1)) if match else 10_000


def chapter_introduction(chapter_dir: Path) -> str:
    notes = sorted(chapter_dir.glob("item*.md"), key=lambda path: (item_number(path), natural_key(path)))
    if not notes:
        return ""

    lines = notes[0].read_text(encoding="utf-8").replace("\r\n", "\n").splitlines()
    first_item_heading = next(
        (index for index, line in enumerate(lines) if re.match(r"^##\s+", line)),
        len(lines),
    )
    introduction: list[str] = []
    for line in lines[:first_item_heading]:
        if re.match(r"^#\s+", line):
            continue
        fence = re.match(r"^(\s*)(`{4,})(.*)$", line)
        if fence:
            remainder = fence.group(3).lstrip("`").strip()
            introduction.append(f"{fence.group(1)}```{remainder}")
        else:
            introduction.append(line)
    return "\n".join(introduction).strip()


def chapter_material(chapter_dir: Path) -> tuple[list[str], int, int]:
    output: list[str] = []
    code_files = sorted(
        [path for path in chapter_dir.iterdir() if path.is_file() and path.suffix.lower() in {".cpp", ".h", ".hpp"}],
        key=natural_key,
    )
    images = sorted(chapter_dir.rglob("*.png"), key=lambda path: (item_number(path), natural_key(path)))

    basic_code = [path for path in code_files if not path.name.lower().startswith("item")]
    item_code = [path for path in code_files if path.name.lower().startswith("item")]
    basic_images = [path for path in images if not path.name.lower().startswith("item")]
    item_images = [path for path in images if path.name.lower().startswith("item")]

    introduction = chapter_introduction(chapter_dir)
    if introduction:
        output.extend(["### 章节说明", "", introduction, ""])

    if basic_code or basic_images:
        output.extend(["### 基础代码与图解", ""])
        used_images: set[Path] = set()
        for source_file in basic_code:
            matching = [
                image for image in basic_images
                if image.stem == source_file.stem or image.stem.startswith(f"{source_file.stem}_")
            ]
            for image in matching:
                output.extend([image_block(image, 4), ""])
                used_images.add(image)
            output.extend([code_block(source_file), ""])
        for image in basic_images:
            if image not in used_images:
                output.extend([image_block(image, 4), ""])
    if item_code or item_images:
        output.extend(["### 条款代码与图解", ""])
        item_numbers = sorted({item_number(path) for path in [*item_code, *item_images]})
        for number in item_numbers:
            if number == 10_000:
                continue
            output.extend([f"#### Item {number} 示例", ""])
            for image in [path for path in item_images if item_number(path) == number]:
                output.extend([image_block(image), ""])
            for source_file in [path for path in item_code if item_number(path) == number]:
                output.extend([code_block(source_file, 5), ""])

    return output, len(code_files), len(images)


def main() -> None:
    if not SOURCE_DIR.is_dir():
        raise FileNotFoundError(SOURCE_DIR)

    if ASSET_DIR.exists():
        shutil.rmtree(ASSET_DIR)
    ASSET_DIR.mkdir(parents=True, exist_ok=True)

    output = [
        "# 现代 C++",
        "",
        "> **资料来源：**[Effective Modern C++ 中文翻译](https://github.com/kelthuzadx/EffectiveModernCppChinese)　｜　内容来自本地保存的项目资料。",
        ">",
        "> 本课程按本地项目目录整理：每章先展示章节说明，再展示基础代码、条款代码与配套图解；具体条款正文不再收录，图片路径均已适配为本地离线资源。",
        "",
    ]

    imported_code = 0
    imported_images = 0
    for chapter_title, directory in CHAPTERS:
        output.extend([f"## {chapter_title}", ""])
        material, code_count, image_count = chapter_material(SOURCE_DIR / directory)
        output.extend(material)
        imported_code += code_count
        imported_images += image_count

    OUTPUT_FILE.write_text("\n".join(output).rstrip() + "\n", encoding="utf-8")
    copied_images = sum(1 for path in ASSET_DIR.iterdir() if path.is_file())
    print(f"已导入 {imported_code} 个 C++/头文件，已排除全部 CMake 配置")
    print("已保留每章说明，移除全部具体条款和课程简介")
    print(f"已整理 {imported_images} 张图片，共复制 {copied_images} 张课程图片")
    print(f"输出文件：{OUTPUT_FILE}")


if __name__ == "__main__":
    main()
