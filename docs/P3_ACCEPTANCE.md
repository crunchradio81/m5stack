# P3 Offline Pocket Tools — original K132 acceptance

Status: **source/CI checks only** until flashed on an actual original Cardputer. No Grove attachments, SD card, network connection or external sensors are required.

## Stopwatch
- [ ] Launch Stopwatch from Home and verify the running display updates without clipping.
- [ ] S/Enter starts and pauses; a second start resumes the accumulated value.
- [ ] L captures laps while running; the newest three appear on screen.
- [ ] Retains the last eight lap times without exhausting memory.
- [ ] R while running refuses to erase the stopwatch; R while paused resets.
- [ ] Leave to Home and reopen Stopwatch; verify elapsed time continues to represent real elapsed time.
- [ ] Test across a 32-bit millisecond counter rollover (simulated by host tests; long real run optional).

## Countdown
- [ ] Countdown opens with 5-minute preset.
- [ ] Type 0130 then Enter: timer sets 01:30; S starts, S pauses, S resumes.
- [ ] R resets the configured interval; does not erase other application state.
- [ ] 000000, 0060, 999999, out-of-range characters are rejected.
- [ ] Completion is shown visually; it is **not** yet a sound alarm.
- [ ] Leaving the screen does not require an RTC; elapsed time uses uptime only.
- [ ] After a cold boot the countdown is intentionally **not** restored.
- [ ] Verify timing within normal ESP32 millis()/display scheduling resolution.

## Unit Converter
- [ ] Fn Up/Down scroll through 16 conversions.
- [ ] Input integers, negative values and decimal fractions, Backspace edits, C clears.
- [ ] Check 0 C = 32 F, 100 C = 212 F, 1 mi = 1.609344 km.
- [ ] Check US liquid gallons, NOT Imperial gallons.
- [ ] Reject physically invalid temperatures below absolute zero.
- [ ] Invalid numeric input does not crash or display a misleading result.

## Programmer Calculator
- [ ] H cycles binary, octal, decimal and hexadecimal and preserves value.
- [ ] T changes A/B operand focus, O changes AND/OR/XOR/NOT/SHL/SHR.
- [ ] Binary, decimal, hexadecimal outputs match an independent calculator.
- [ ] Test 32-bit unsigned overflow; 4294967296 and invalid digits are refused.
- [ ] Left/right shifts >=32 are rejected, not executed.
- [ ] Test 0, 0xFFFFFFFF, 1<<31, bitwise NOT.
- [ ] Backspace deletes, R clears fields, Escape/Home exits safely.

## Combined suite
- [ ] Scrollable Home menu reaches all 12 entries.
- [ ] Notes, MIDI Studio and Wi-Fi Logger still launch/exit after using all four tools.
- [ ] CPU and heap remain stable across repeated app switching.
- [ ] Firmware is built for original no-PSRAM 8MB-flash K132.

Record firmware commit SHA, board model, test steps, serial log, any failures and photos of each screen before marking a release hardware verified.
