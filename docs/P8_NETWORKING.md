# P8 — Network & Internet SD Collection (original K132)

**Experimental: firmware compilation and host tests are not hardware acceptance.**

The original stock Cardputer K132 has 2.4 GHz Wi-Fi but no Ethernet,
cellular, GPS, PSRAM or 5/6 GHz radio. P8 adds **one** SD firmware image
`NETTOOLS.BIN` with **nine screens**:

1. Wi-Fi Connection: load an explicit SD profile; connect/disconnect.
   Connection timeout 12 seconds. Shows SSID, local address and RSSI.
2. DNS Lookup: resolve configured `DNS_HOST` and display the IPv4 answer.
3. ICMP Ping: one genuine ICMP echo using ESP-IDF ping socket interface,
   with a 1.5-second timeout and reported round-trip latency.
4. NTP UTC Clock: synchronize via public NTP servers and show UTC time.
   This is unauthenticated NTP, not a tamper-proof time source.
5. HTTP Text GET: explicit `http://` endpoint, bounded response display.
   **HTTP is unencrypted; HTTPS with verified certificates is not
   implemented.** No credentials or private information should be sent.
6. LAN mDNS Discovery: query `_http._tcp` services on the current LAN.
7. MQTT Subscriber: connect to a **trusted local plaintext broker**
   without username/password/TLS and subscribe to a user-defined topic.
   It only receives; no arbitrary MQTT publishing or broker credentials.
8. LAN UDP Chat: listen/send broadcast packets on UDP 38945. Messages
   limited to 72 printable ASCII chars; input is treated as text only.
   This **is not encrypted or authenticated**.
9. RSS Title Reader: show the first basic XML `<title>` in a configured
   HTTP feed within the first 1 KiB. Not a full RSS/Atom parser, no
   HTML rendering or HTTPS support in P8.

## SD configuration (user creates; never check in credentials!)

Create `/SUITE/NET/NET.CFG` on FAT32 SD:

~~~text
K132NET1
SSID=Your WiFi SSID
PASSWORD=your wifi password
DNS_HOST=example.com
TCP_HOST=example.com
TCP_PORT=80
HTTP_HOST=192.168.1.10
HTTP_PORT=8000
HTTP_PATH=/status
MQTT_HOST=192.168.1.20
MQTT_PORT=1883
MQTT_TOPIC=cardputer/events
CHAT_NAME=Cardputer
~~~

Only `SSID` is mandatory. `PASSWORD=` can be blank for an open network.
The `HTTP_HOST` and `MQTT_HOST` options are intentionally blank by default,
so no unintended external connections occur. `TCP_HOST` is used as the
ICMP target in this milestone. Invalid/duplicate/overlong fields cause
the entire config to be rejected before connecting.

**Security:** Wi-Fi passwords are stored in **plaintext on removable SD**.
Protect the card. Do not commit the config to GitHub, share it, or leave
the card unattended. Neither HTTP, MQTT nor UDP chat provides TLS or
authentication, so use them only on a trusted private network, for
non-sensitive information. NTP and mDNS traffic may traverse networks.

The device does not start network operations just because the SD card
contains a profile. Open **Wi-Fi Connection** and press S/Enter to
connect; press D to disconnect. B returns to the network submenu.
Other screens require an active Wi-Fi link and initiate work only after
S/Enter. LAN chat listens only while its screen is open.

## Memory/latency limits

P8 bounds configuration to 1,024 bytes, HTTP body buffer to 1,024 bytes
and user-visible response to a fraction of that, LAN messages to 72 ASCII
characters, DNS/mDNS to single query, MQTT display to 80 bytes and ICMP
to one echo. Some network operations have bounded **seconds-long**
synchronous timeouts and can pause the UI temporarily; this is not a
background web browser or general-purpose Internet appliance.

No persistent browsing, GPS, port scanning, traffic interception, WLAN
client enumeration or other users' private information is supported.

## How it fits the suite

`NETTOOLS.APP` plus `NETTOOLS.BIN` are packaged with the prior 14
SD apps. The factory suite is unchanged: the selected image is
installed into the replaceable OTA partition. **It cannot execute
directly from SD.** The source uses the original K132 firmware profile
(`espressif32@6.7.0` with no PSRAM), and must return to factory after
an app restart. See `docs/SD_LAUNCHER.md` for first-time flash erasure
and recovery warnings, and `docs/P8_ACCEPTANCE.md` for hardware checks.

Future networking catalog work: authenticated HTTPS, verified CA store,
secure MQTT, full RSS/Atom, LAN file transfer with authorization,
smart-home integrations, Wikipedia text reader, Internet radio and
more thorough diagnostics. Do not claim those are implemented.
