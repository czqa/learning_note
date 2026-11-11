#!/usr/bin/env python3
"""Copy remote Linux-course images into dist and rewrite Markdown links.

The imported articles keep their original attribution.  This helper only
changes image storage so the local study site can display them offline.
"""

from __future__ import annotations

import hashlib
import mimetypes
import re
import shutil
import urllib.parse
import urllib.request
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path


PROJECT_DIR = Path(__file__).resolve().parent
NOTE_FILE = PROJECT_DIR / "dist" / "aux-notes.md"
ASSET_DIR = PROJECT_DIR / "dist" / "assets" / "linux-course"
DOWNLOAD_DIR = Path("/home/czq/下载")
IMAGE_PATTERN = re.compile(
    r"!\[[^\]]*\]\((https://subingwen\.cn/[^)\s]+)\)"
)


def safe_name(value: str) -> str:
    value = urllib.parse.unquote(value)
    value = re.sub(r"[^0-9A-Za-z._-]+", "-", value).strip("-.")
    return value or "image"


def image_suffix(content_type: str, data: bytes) -> str:
    content_type = content_type.split(";", 1)[0].strip().lower()
    suffix = (
        mimetypes.guess_extension(content_type) or ""
        if content_type.startswith("image/")
        else ""
    )
    if suffix == ".jpe":
        suffix = ".jpg"
    if suffix:
        return suffix
    signatures = (
        (b"\x89PNG\r\n\x1a\n", ".png"),
        (b"\xff\xd8\xff", ".jpg"),
        (b"GIF87a", ".gif"),
        (b"GIF89a", ".gif"),
        (b"RIFF", ".webp"),
    )
    for signature, detected in signatures:
        if data.startswith(signature):
            return detected
    if data.lstrip().startswith(b"<svg"):
        return ".svg"
    return ""


def build_local_index() -> dict[str, Path]:
    index: dict[str, Path] = {}
    if not DOWNLOAD_DIR.is_dir():
        return index
    for path in DOWNLOAD_DIR.rglob("*"):
        if path.is_file():
            index.setdefault(path.name, path)
    return index


def fetch_image(url: str, local_index: dict[str, Path]) -> tuple[str, str]:
    parsed = urllib.parse.urlparse(url)
    original_name = Path(urllib.parse.unquote(parsed.path)).name
    digest = hashlib.sha256(url.encode("utf-8")).hexdigest()[:12]
    base_name = safe_name(original_name)
    suffix = Path(base_name).suffix
    target_name = f"remote-{digest}-{base_name}"

    local_source = local_index.get(original_name)
    if local_source:
        target = ASSET_DIR / target_name
        shutil.copy2(local_source, target)
        return url, f"assets/linux-course/{target.name}"

    request_url = urllib.parse.urlunsplit(
        (
            parsed.scheme,
            parsed.netloc,
            urllib.parse.quote(parsed.path, safe="/%"),
            parsed.query,
            parsed.fragment,
        )
    )
    request = urllib.request.Request(
        request_url,
        headers={
            "User-Agent": "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36",
            "Referer": "https://subingwen.cn/",
        },
    )
    with urllib.request.urlopen(request, timeout=30) as response:
        data = response.read()
        content_type = response.headers.get("Content-Type", "")

    detected_suffix = image_suffix(content_type, data)
    if not content_type.lower().startswith("image/") and not detected_suffix:
        raise ValueError(f"返回的不是图片：{content_type or '未知类型'}")
    if not suffix and detected_suffix:
        target_name += detected_suffix

    target = ASSET_DIR / target_name
    target.write_bytes(data)
    return url, f"assets/linux-course/{target.name}"


def main() -> None:
    markdown = NOTE_FILE.read_text(encoding="utf-8")
    urls = sorted(set(IMAGE_PATTERN.findall(markdown)))
    if not urls:
        print("没有需要本地化的远程图片。")
        return

    ASSET_DIR.mkdir(parents=True, exist_ok=True)
    local_index = build_local_index()
    replacements: dict[str, str] = {}
    failures: list[tuple[str, str]] = []

    with ThreadPoolExecutor(max_workers=8) as executor:
        tasks = {
            executor.submit(fetch_image, url, local_index): url for url in urls
        }
        for task in as_completed(tasks):
            url = tasks[task]
            try:
                source, local_path = task.result()
                replacements[source] = local_path
                print(f"已保存：{source} -> {local_path}")
            except Exception as error:  # report every failed URL together
                failures.append((url, str(error)))
                print(f"失败：{url} ({error})")

    if failures:
        print(f"\n仍有 {len(failures)} 张图片未能保存，未修改笔记文件。")
        raise SystemExit(1)

    for source, local_path in replacements.items():
        markdown = markdown.replace(source, local_path)
    NOTE_FILE.write_text(markdown, encoding="utf-8")
    print(f"\n完成：{len(replacements)} 张远程图片已改为本地资源。")


if __name__ == "__main__":
    main()
