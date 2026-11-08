# P21 — Playlists, RSS item browsing and LAN SSDP discovery

Original stock Cardputer K132 only; 8MB flash, 240x135 display, no PSRAM or Grove.
This is source/CI development pending original device validation.

## MIDI playlist manager in resident factory player
The existing MIDI Studio now has an ordered, editable 24-song queue.
A in the library adds the selected .MID/.MIDI/.KAR file, Q enters the queue.
Inside the queue, arrow keys select, U/J move, D deletes, S explicitly saves,
R reloads when unchanged, P toggles playlist playback order, Enter plays
the selected queued song and enables order; B returns to the player.
During playback Q shows the queue; N and the existing A auto-next follow
the queue when enabled. Per-song loop takes precedence.
Saved format: /SUITE/MIDI/PLAYLIST.PLS, signature K132PLAY1, bounded ASCII
names, no paths, duplicates or control bytes; at most 1664 bytes / 24 songs.
S writes PLAYLIST.NEW, verifies exact bytes, rotates PLAYLIST.BAK and reads
back the finished file. Damaged saved files, interrupted .NEW and backup-only
states are read-only until manual recovery. FAT32 is not power-loss-safe.

## Verified HTTPS RSS and Atom
WEBSECURE continues to require owner NET.CFG and WEB.CFG, a CA PEM, NTP,
certificate verification and explicit GETs (no insecure mode/redirects).
Its RSS reader now extracts up to eight complete item/entry titles from
the first 3072 bytes, supports both RSS2 and Atom, strips embedded markup,
bounds titles and handles basic XML entities / CDATA.
Up/down browses the retained titles, S refetches, B returns.
No full articles, archive, attachments, large/complex feed support or
automatic background requests are claimed.

## User-started SSDP / UPnP discovery
FIELD2 gains a fourth mode: LAN SSDP Discovery.
After explicit owner NET.CFG Wi-Fi connection, S sends a single M-SEARCH
multicast to 239.255.255.250:1900, collects for about 3.2 seconds,
accepts only bounded HTTP 200 responses from RFC1918 source addresses,
and displays up to twelve unique service type/IP pairs.
No UPnP commands, port scanning, follow-on advertised URL GETs or device
control. R clears, arrows select, B returns and C connects owner Wi-Fi.

## Verification
Portable C++ regression tests cover malformed playlist records, edit/move
logic, RSS and Atom sanitization, and malformed SSDP headers. Physical
original K132 verification remains required: MIDI sustained playback and
queue auto-advance, SD write/power-loss recovery, HTTPS trust failures,
actual discovered LAN devices, OTA factory reboot and USB recovery.
Same original 27 native SD images plus separately packaged GPL C64.
