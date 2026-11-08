#!/usr/bin/env python3
"""Package only P20-adapted upstream C64 OTA image with GPL source.

Refuse direct-flash unmodified third-party binaries and private ROM payloads.
"""
import argparse, hashlib
from pathlib import Path
import zipfile

MAX=0x300000
def package(binary:Path,source:Path,out:Path):
    data=binary.read_bytes()
    if len(data)<64 or len(data)>MAX or data[0]!=0xe9:
        raise ValueError("C64 binary missing, oversize, or not ESP32 app")
    required=["LICENSE","P20_ADAPTER_NOTICE.txt","src/main.cpp",
              "src/t-hmi-c64/roms/basic.h","src/t-hmi-c64/roms/kernal.h",
              "src/t-hmi-c64/roms/charset.h"]
    for path in required:
        if not (source/path).is_file():raise ValueError("Missing adapter/source: "+path)
    main=(source/"src/main.cpp").read_text()
    if "suite_prepare_factory_return()" not in main or "esp_ota_set_boot_partition(factory)" not in main:
        raise ValueError("Refusing image with missing source-level factory-return guard")
    sd=(source/"src/t-hmi-c64/SDCard.cpp").read_text()
    if "SD.begin(PWR_EN_PIN,SPI,10000000)" not in sd:
        raise ValueError("Refusing adapter with wrong microSD bus")
    notice=(source/"P20_ADAPTER_NOTICE.txt").read_text()
    if "User-provided private firmware ROMs" in notice:
        raise ValueError("Never distribute owner-private C64 ROMs")
    digest=hashlib.sha256(data).hexdigest()
    manifest=(f"K132APP1\nNAME=Commodore 64 (Open ROMs)\nID=C64\n"
              f"IMAGE=C64.BIN\nSIZE={len(data)}\nSHA256={digest}\n").encode("ascii")
    readme=("P20 C64 upstream adapter - UNVERIFIED original K132 hardware\n"
            "Only for the stock original Cardputer K132 factory + OTA_0 layout.\n"
            "NOT the author's direct-flash image; includes factory next-boot\n"
            "guard, original K132 SPI SD and matching MEGA65 Open ROMs.\n"
            "No hardware boot, game compatibility or reset proof yet.\n"
            "Do not flash until complete K132 acceptance tests pass.\n"
            "Full GPL upstream source, adapter and ROM notices are under SOURCE/.\n"
            "Upstream: https://github.com/iele/M5Cardputer-C64-Emulator\n"
            "Open ROMs: https://github.com/MEGA65/open-roms\n"
            "Only one SD app resides in OTA_0 at once.\n")
    out.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(out,"w",zipfile.ZIP_DEFLATED) as z:
        z.writestr("README.txt",readme)
        z.writestr("SUITE/APPS/C64.APP",manifest)
        z.writestr("SUITE/APPS/C64.BIN",data)
        for p in sorted(source.rglob("*")):
            if not p.is_file() or set(p.relative_to(source).parts)&{".git",".pio",".pioenvs",".piolibdeps"}:
                continue
            z.write(p,arcname="SOURCE/"+p.relative_to(source).as_posix())
    print(f"Created {out}: C64 app, SHA256 and GPL/Open ROM sources")
def main():
    p=argparse.ArgumentParser()
    p.add_argument("--binary",type=Path,required=True)
    p.add_argument("--source",type=Path,required=True)
    p.add_argument("--out",type=Path,required=True)
    a=p.parse_args()
    package(a.binary,a.source,a.out)
if __name__=="__main__":main()
