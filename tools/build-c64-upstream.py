#!/usr/bin/env python3
"""Build the existing GPL Cardputer C64 port as an isolated K132 OTA app.

Pinned third-party source, same C64 engine; only adapter changes are:
  (1) arm return-to-factory before boot
  (2) use stock K132 SPI SD bus
  (3) replace bundled historical Commodore ROMs with MEGA65 Open ROMs
No GPL emulator source or historical Commodore ROM is checked into this repo.
Use --rom-dir for your own locally-held, legally acquired ROMs instead.
"""
from __future__ import annotations
import argparse
import subprocess
from pathlib import Path
import shutil

ORIGIN="https://github.com/iele/M5Cardputer-C64-Emulator.git"
COMMIT="3cd226097a5b61727dd5e5b0fd07318f4d33fd58"
OPEN="https://github.com/MEGA65/open-roms.git"
OPEN_COMMIT="ad178dbe4d48cd6a317737a8e0e7e662f7e33d32"
MAX=0x300000

def run(*args):
    subprocess.run(args,check=True)

def clone(url, sha, path):
    if path.exists():
        raise ValueError(f"Refusing to overwrite checkout {path}")
    run("git","clone","--quiet",url,str(path))
    run("git","-C",str(path),"checkout","--quiet","--detach",sha)

def replace(path, old, new):
    body=path.read_text()
    if body.count(old)!=1:
        raise ValueError(f"Expected exact upstream pattern once in {path}: {old[:80]!r}")
    path.write_text(body.replace(old,new))

def rom_header(data:bytes,variable:str)->str:
    return (f"// Generated from owner-supplied or MEGA65 Open ROM, not Commodore originals.\n"
            f"unsigned char {variable}[] = {{\n" +
            "".join("  "+", ".join(f"0x{b:02x}" for b in data[i:i+16])+",\n"
                    for i in range(0,len(data),16))+
            "};\n")

def prepare(work:Path, owner_roms:Path|None=None):
    work.mkdir(parents=True,exist_ok=True)
    repo=work/"source"
    clone(ORIGIN,COMMIT,repo)
    if owner_roms:
        candidates=[owner_roms/"basic.rom",owner_roms/"kernal.rom",owner_roms/"chargen.rom"]
        if not all(p.exists() for p in candidates):
            raise ValueError("Private ROM folder must include basic.rom, kernal.rom and chargen.rom")
        license_summary="User-provided private firmware ROMs; NOT for redistribution"
    else:
        openrom=work/"open-roms"
        clone(OPEN,OPEN_COMMIT,openrom)
        candidates=[openrom/"bin/basic_generic.rom",
                    openrom/"bin/kernal_generic.rom",
                    openrom/"bin/chargen_openroms.rom"]
        license_summary="MEGA65 Open ROMs; GPL-3.0-or-later / LGPL-3.0-or-later"
        # Open ROMs is intentionally a matching Generic BASIC+KERNAL pair.
    sizes=[8192,8192,4096]
    for p,n,var,name in zip(candidates,sizes,["basic_rom","kernal_rom","charset_rom"],
                             ["basic","kernal","charset"]):
        data=p.read_bytes()
        if len(data)!=n:raise ValueError(f"Invalid {name} size {len(data)} != {n}")
        (repo/f"src/t-hmi-c64/roms/{name}.h").write_text(rom_header(data,var))
    sd=repo/"src/t-hmi-c64/SDCard.cpp"
    replace(sd,"#include \"SDCard.h\"","#include \"SDCard.h\"\n#include <SD.h>\n#include <SPI.h>")
    replace(sd,'''  pinMode(PWR_EN_PIN, OUTPUT);
  digitalWrite(PWR_EN_PIN, HIGH);
  SD_MMC.setPins(SD_SCLK_PIN, SD_MOSI_PIN, SD_MISO_PIN);
  bool rlst = SD_MMC.begin("/sdcard", true);''',
            '''  // Stock K132: SD is SPI with GPIO12 chip-select, not SD_MMC.
  SPI.begin(SD_SCLK_PIN,SD_MISO_PIN,SD_MOSI_PIN,PWR_EN_PIN);
  bool rlst = SD.begin(PWR_EN_PIN,SPI,10000000);''')
    replace(repo/"src/t-hmi-c64/SDCard.h","#include <SD_MMC.h>","#include <SD.h>")
    replace(repo/"src/sd.cpp","#include <SD_MMC.h>","#include <SD.h>")
    for path in [repo/"src/sd.cpp",repo/"src/t-hmi-c64/C64.cpp"]:
        body=path.read_text()
        if "SD_MMC" not in body:raise ValueError(f"Upstream SD_MMC reference absent {path}")
        path.write_text(body.replace("SD_MMC","SD"))
    main=repo/"src/main.cpp"
    replace(main,"#include <esp_log.h>","""#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
static bool suite_prepare_factory_return() {
  const esp_partition_t *running=esp_ota_get_running_partition();
  const esp_partition_t *factory=esp_partition_find_first(
      ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY,nullptr);
  if (!running || !factory ||
      running->subtype!=ESP_PARTITION_SUBTYPE_APP_OTA_0 ||
      running->address==factory->address) return false;
  return esp_ota_set_boot_partition(factory)==ESP_OK;
}
static bool suite_return_armed=false;""")
    replace(main,"  disableCore0WDT();","""  suite_return_armed=suite_prepare_factory_return();
  if (!suite_return_armed) {
    Serial.begin(115200);
    Serial.println("C64: factory next-boot guard failed; refusing to run");
    return;
  }
  disableCore0WDT();""")
    replace(main,"  String path = menu();",
            """  if(!suite_return_armed){vTaskDelay(500 / portTICK_PERIOD_MS);return;}
  String path = menu();""")
    # Keep direct upstream code/board, but standardize K132 flash and partition map.
    partition=Path(__file__).resolve().parents[1]/"partitions/suite_8mb_factory.csv"
    shutil.copy2(partition,repo/"suite_8mb_factory.csv")
    ini=repo/"platformio.ini"
    ini.write_text(f"""[platformio]
default_envs = c64_suite
[env:c64_suite]
platform = espressif32@6.7.0
board = esp32-s3-devkitc-1
framework = arduino
board_build.flash_size = 8MB
board_upload.flash_size = 8MB
board_build.partitions = suite_8mb_factory.csv
board_upload.maximum_size = {MAX}
monitor_speed = 115200
build_flags =
    -O2
    -DESP32S3
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DARDUINO_USB_MODE=1
lib_deps =
    M5Cardputer=https://github.com/m5stack/M5Cardputer
    xylopyrographer/LiteLED@^1.2.0
""")
    (repo/"P20_ADAPTER_NOTICE.txt").write_text(
        "Upstream iEle M5Cardputer C64 (GPL-3.0) commit "+COMMIT+"\n"
        "Code-based on retroelec T-HMI-C64. The adapter replaces historical\n"
        "Commodore ROM headers; preserves license headers; uses K132 SPI SD\n"
        "and boots once from OTA_0 before arming the factory for next reboot.\n"
        "ROM source: "+license_summary+"\n"
        "Generated adapters are inspectable in this source checkout.\n"
        "Actual original K132 hardware operation NOT VERIFIED.\n")
    return repo

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--work",type=Path,required=True)
    p.add_argument("--rom-dir",type=Path,help="Owner-private basic.rom/kernal.rom/chargen.rom")
    p.add_argument("--prepare-only",action="store_true")
    p.add_argument("--out-dir",type=Path)
    args=p.parse_args()
    if args.rom_dir and args.out_dir:
        raise SystemExit("Do not package owner-private ROM data for redistribution.")
    repo=prepare(args.work,args.rom_dir)
    print("Pinned upstream C64 source adapted:",repo)
    if args.prepare_only:return
    run("pio","run","-d",str(repo),"-e","c64_suite")
    binary=repo/".pio/build/c64_suite/firmware.bin"
    size=binary.stat().st_size
    if size<64 or size>MAX or binary.read_bytes()[0]!=0xe9:
        raise ValueError(f"Not valid 3MiB ESP32-S3 app: {size} bytes")
    print(f"C64 OTA application compiled: {binary} ({size} bytes)")
    if args.out_dir:
        args.out_dir.mkdir(parents=True,exist_ok=True)
        shutil.copy2(binary,args.out_dir/"C64.BIN")
        shutil.copy2(repo/"P20_ADAPTER_NOTICE.txt",args.out_dir/"NOTICE.txt")
        # Do not ship owner-provided ROMs, and do not silently certify hardware.
if __name__=="__main__":main()
