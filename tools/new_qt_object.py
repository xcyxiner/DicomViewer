#!/usr/bin/env python3
import sys
import pathlib

CLASS_NAME = sys.argv[1]
BASE_CLASS = sys.argv[2]  # QWidget / QMainWindow / QDialog
NAMESPACE = sys.argv[3] if len(sys.argv) > 3 else ""

HEADER = f"""
#pragma once
#include <{BASE_CLASS}>

class {CLASS_NAME} : public {BASE_CLASS}
{{
    Q_OBJECT
public:
    explicit {CLASS_NAME}(QWidget *parent = nullptr);
}};
"""

CPP = f"""
#include "{CLASS_NAME}.h"

{CLASS_NAME}::{CLASS_NAME}(QWidget *parent)
    : {BASE_CLASS}(parent)
{{
}}
"""

pathlib.Path(f"{CLASS_NAME}.h").write_text(HEADER)
pathlib.Path(f"{CLASS_NAME}.cpp").write_text(CPP)

print(f"✅ Generated {CLASS_NAME}.h / {CLASS_NAME}.cpp")

