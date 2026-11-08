# Cardputer Suite (original K132)

Native application suite for the **original M5Stack Cardputer** (2023 K132 / StampS3). **No Grove attachments or hardware add-ons.** Optional microSD uses the built-in slot.

## Status

Development repository initialized for the P1 source and hardware-verification work. **Not yet a verified firmware release**; do not flash without first preserving your device firmware/data.

## Hardware baseline

- ESP32-S3, 8 MB flash, **no PSRAM**.
- Built-in 240×135 display, keyboard, microSD, Wi-Fi, Bluetooth LE, microphone, speaker and IR transmitter.
- Native PlatformIO / Arduino / M5Cardputer toolchain.
- No Cardputer-ADV-only sensors, codecs or expansion-unit dependencies.

Source and build instructions will be maintained here. Milestones are reviewed through pull requests.

Hardware reference: https://docs.m5stack.com/en/core/Cardputer
