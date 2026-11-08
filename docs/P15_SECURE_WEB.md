# P15 — Verified HTTPS Web Tools, original K132

`WEBSECURE.APP`/`WEBSECURE.BIN` is a **single** suite-managed OTA
application with six screens: Wi-Fi/CA status, HTTPS Text Browser,
HTTPS RSS Reader, Wikipedia Text, Weather Dashboard (manually supplied
coordinates) and HTTPS API GET.

**TLS is fail closed.** Each request requires all of:
1. A valid user-created `/SUITE/NET/NET.CFG` Wi-Fi configuration
   (same format as P8, stores password in **plaintext on removable SD**).
2. A valid `/SUITE/WEB/WEB.CFG` configuration for endpoint choices.
3. An owner-supplied PEM root CA certificate in
   `/SUITE/WEB/ROOT_CA.PEM` of 500–4096 bytes.
4. An actual connected Wi-Fi link, valid NTP-synchronized clock, and
   an HTTPS server whose certificate chain verifies against the root CA.

The firmware always configures `WiFiClientSecure.setCACert`. It
**never calls `setInsecure`**, never falls back to HTTP, never follows
redirects, and refuses to connect without a certificate/clock.

## Example WEB.CFG

~~~text
K132WEB1
WEB_HOST=example.org
WEB_PATH=/index.html
RSS_HOST=feeds.example.org
RSS_PATH=/rss.xml
API_HOST=api.example.org
API_PATH=/v1/status
WIKI_TITLE=Computer history
LATITUDE=33.4484
LONGITUDE=-112.0740
~~~

Examples are **format samples, not verified live endpoints**. The
coordinates are illustrative; use your own manually selected point if
you want weather. The device doesn't infer GPS/location and never
uploads the user's precise device position implicitly. Weather uses
Open-Meteo's API with latitude and longitude explicitly supplied by
the user; Wikipedia summary uses the MediaWiki extracts API. These
services may change their APIs or limits, so availability and output
must be checked during original-board acceptance.

Only host/path values of valid size and syntax are accepted; user
queries cannot inject raw HTTP headers, a scheme or URL fragment.
Requests happen **only on S/Enter**, never automatically when the card
is inserted. **D** disconnects Wi-Fi in the Wi-Fi screen, **R** reloads
configs, **B** returns to the menu. Escape/backtick returns to factory
launcher through the suite-owned OTA guard.

## Functional limits

- Built-in 2.4GHz Wi-Fi only. No PSRAM, camera, GPS or expansion board.
- HTTPS GET only, 3KiB response limit, 3-second connection/read timeouts.
- Text Browser strips basic HTML tags and entities from early content
  but does not execute JavaScript, render CSS, follow links or support
  a multi-page interactive browser.
- RSS Reader shows the first `<item><title>` or
  `<entry><title>` in the downloaded prefix. It is not a complete
  XML/RSS/Atom feed parser or persistent feed manager.
- Wikipedia Reader shows `extract` text if it appears in the first
  3KiB and uses bounded ASCII JSON decoding. JSON Unicode escapes
  are not supported in this version.
- Weather displays current temperature, relative humidity and wind
  when recognized JSON fields appear; no forecast graphs or GPS.
- API GET displays bounded, sanitized response text; it does not POST,
  PUT, PATCH, authenticate accounts or send private data automatically.
- Responses with compression/chunking beyond this minimal handling
  may not be readable. The owner must supply root CA chain appropriate
  to each target server, and trusted bundle changes require SD updates.
- Certificate chain validity depends on NTP. The SD CA file itself
  must be obtained from a trusted source; this does not protect a
  compromised SD card or altered factory firmware.

The app extends the suite to **22 SD firmware packages** after P15.
Custom factory+OTA partitions are unchanged. Initial adoption still
needs owner-approved backup and one-time destructive flash erase.
**No original K132 physical acceptance has been performed.**
See `docs/P15_ACCEPTANCE.md`.
