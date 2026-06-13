import os

src = []
for root, _, files in os.walk("source"):
    for f in files:
        if f.endswith(".cpp"):
            src.append(os.path.join(root, f))

with open("source_list.cmake", "w") as f:
    f.write("set(SRC\n")
    for s in src:
        f.write(f"    {s}\n")
    f.write(")\n")
