import {readFile, writeFile} from "node:fs/promises";
import {dirname, join} from "node:path";
import {fileURLToPath} from "node:url";

const projectRoot = dirname(fileURLToPath(import.meta.url));
const distDirectory = join(projectRoot, "dist");
const main = await readFile(join(distDirectory, "note.md"), "utf8");
const modern = await readFile(join(distDirectory, "modern-cpp.md"), "utf8");
const auxiliary = await readFile(join(distDirectory, "aux-notes.md"), "utf8");
const cudaInterview = await readFile(join(distDirectory, "cuda-interview.md"), "utf8");
const output = `// 由 build-note-data.mjs 自动生成，用于支持直接双击打开网站。\nwindow.STUDY_NOTE_DATA = ${JSON.stringify({main, modern, auxiliary, cudaInterview})};\n`;

await writeFile(join(distDirectory, "note-data.js"), output, "utf8");
console.log("已生成 dist/note-data.js");
