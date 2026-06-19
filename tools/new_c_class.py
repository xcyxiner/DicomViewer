#!/usr/bin/env python3
import sys
import pathlib

CLASS_NAME = sys.argv[1]

HEADER = f"""
#pragma once

class {CLASS_NAME} 
{{
public:
    explicit {CLASS_NAME}();
    ~{CLASS_NAME}()=default;
}};
"""

CPP = f"""
#include "{CLASS_NAME}.h"

{CLASS_NAME}::{CLASS_NAME}()
{{
}}
"""

pathlib.Path(f"{CLASS_NAME}.h").write_text(HEADER)
pathlib.Path(f"{CLASS_NAME}.cpp").write_text(CPP)

print(f"✅ Generated {CLASS_NAME}.h / {CLASS_NAME}.cpp")

