#!/usr/bin/env python3
"""生成多层轴位 DICOM 序列测试夹具 (res/series/)。

从现有单文件样本 res/CT_small.dcm 派生 N 层序列：
  - 统一 SeriesInstanceUID，逐层唯一 SOPInstanceUID / InstanceNumber
  - ImagePositionPatient 沿层法线逐层递增 (层厚取样本 SliceThickness)
  - 文件名与 IPP 层序打乱 (固定种子)，用于断言 reader 的 IPP 排序
  - 附带 WindowWidth/WindowCenter，供显示设置断言使用
  - 输出 res/series/expected.txt 作为测试期望 (可再生、确定性)

依赖: pydicom (pip install pydicom)
用法: python3 tools/gen_series_fixture.py [--n 12]
"""

import argparse
import copy
import random
import sys
from pathlib import Path

try:
    import pydicom
except ImportError:
    sys.exit("需要 pydicom: pip install pydicom")

# 确定性 UID 根 (2.25.<UUID 十进制> 形式)，保证夹具可再生且 diff 稳定
SERIES_UID = "2.25.3141592653589793238462643383279"
WW = "400"
WC = "40"

REPO_ROOT = Path(__file__).resolve().parent.parent
BASE_FILE = REPO_ROOT / "res" / "CT_small.dcm"
OUT_DIR = REPO_ROOT / "res" / "series"
EXPECTED = OUT_DIR / "expected.txt"


def build_slices(n: int):
    """按 IPP 升序构造 n 层的期望元数据。"""
    base = pydicom.dcmread(str(BASE_FILE))
    ipp = [float(v) for v in base.ImagePositionPatient]
    iop = [float(v) for v in base.ImageOrientationPatient]
    row, col = iop[:3], iop[3:]
    normal = [
        row[1] * col[2] - row[2] * col[1],
        row[2] * col[0] - row[0] * col[2],
        row[0] * col[1] - row[1] * col[0],
    ]
    thickness = float(getattr(base, "SliceThickness", 1.0)) or 1.0

    slices = []
    for i in range(n):
        slices.append(
            {
                "ipp": [ipp[0] + normal[0] * thickness * i,
                        ipp[1] + normal[1] * thickness * i,
                        ipp[2] + normal[2] * thickness * i],
                "sop_uid": f"{SERIES_UID}.{i + 1}",
                "instance_number": i + 1,
            }
        )
    return base, slices


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--n", type=int, default=12, help="序列层数 (8-16)")
    args = parser.parse_args()
    if not 8 <= args.n <= 16:
        sys.exit("层数需在 8-16 之间")

    base, slices = build_slices(args.n)
    base_uid = str(base.SOPInstanceUID)

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for old in OUT_DIR.glob("*.dcm"):
        old.unlink()

    # 固定种子打乱文件名分配，使文件名字典序 ≠ IPP 层序
    perm = list(range(args.n))
    random.Random(42).shuffle(perm)

    for slice_idx, file_idx in enumerate(perm):
        ds = copy.deepcopy(base)
        ds.ImagePositionPatient = slices[slice_idx]["ipp"]
        ds.SeriesInstanceUID = SERIES_UID
        ds.SOPInstanceUID = slices[slice_idx]["sop_uid"]
        ds.InstanceNumber = slices[slice_idx]["instance_number"]
        ds.WindowWidth = WW
        ds.WindowCenter = WC
        ds.file_meta.MediaStorageSOPInstanceUID = slices[slice_idx]["sop_uid"]
        out = OUT_DIR / f"slice_{file_idx:02d}.dcm"
        pydicom.dcmwrite(str(out), ds, enforce_file_format=True)

    with EXPECTED.open("w", encoding="utf-8") as f:
        f.write(f"series_uid={SERIES_UID}\n")
        f.write(f"count={args.n}\n")
        f.write(f"base_uid={base_uid}\n")
        f.write(f"rows={base.Rows}\n")
        f.write(f"cols={base.Columns}\n")
        f.write(f"pixel_spacing_x={float(base.PixelSpacing[0])}\n")
        f.write(f"pixel_spacing_y={float(base.PixelSpacing[1])}\n")
        f.write(f"slice_thickness={float(base.SliceThickness)}\n")
        f.write(f"window_width={WW}\n")
        f.write(f"window_center={WC}\n")
        f.write(f"orientation={' '.join(str(float(v)) for v in base.ImageOrientationPatient)}\n")
        f.write(f"ipp_x={float(slices[0]['ipp'][0])!r}\n")
        f.write(f"ipp_y={float(slices[0]['ipp'][1])!r}\n")
        for i, s in enumerate(slices):
            f.write(f"ipp_z_{i}={s['ipp'][2]!r}\n")
            f.write(f"uid_{i}={s['sop_uid']}\n")

    print(f"生成 {args.n} 层夹具到 {OUT_DIR}")


if __name__ == "__main__":
    main()
