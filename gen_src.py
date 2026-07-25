from pathlib import Path

src = [
    p.as_posix()   # 直接输出正斜杠格式
    for p in Path("source").rglob("*")
    if p.is_file() and p.suffix in (".cpp", ".h", ".ui")
]

with open("source_list.cmake", "w", newline="\n") as f:
    f.write("set(SRC\n")
    for s in src:
        f.write(f'    "{s}"\n')
    f.write(")\n")