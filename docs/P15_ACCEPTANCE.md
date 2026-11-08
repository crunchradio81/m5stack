# P15 verified HTTPS original K132 physical acceptance — OPEN

- [ ] Confirm stock original K132, no PSRAM/Grove; back up firmware/NVS/SD.
- [ ] WEBSECURE appears in factory launcher, OTA0 installation and escape
      return-to-factory successfully demonstrated.
- [ ] No Wi-Fi connection without a valid owner NET.CFG.
- [ ] Missing/truncated/invalid CA PEM must refuse TLS; wrong-root,
      expired-cert, hostname-mismatch and NTP-unsynced connections fail closed.
- [ ] Valid HTTPS GET to owner-chosen public site with validated CA and NTP.
- [ ] 3KiB response bounds and 3s timeouts work for chunked, long and
      malformed network responses without heap exhaustion or UI hangs.
- [ ] RSS title extraction from a small sample HTTPS feed.
- [ ] Wikipedia summary with ASCII extract from fixed title; no arbitrary
      outbound sensitive data or script execution.
- [ ] Weather with manual test coordinates returns expected current units.
- [ ] Sanitized API GET reports non-2xx, rejected redirects and oversize
      bodies honestly.
- [ ] Original K132 battery/heap stability across 100 HTTPS requests.
- [ ] CA buffer lifetime remains valid for entire TLS client session.
- [ ] Screenshots/UART, code SHA, CA fingerprint and SD sample test recorded.
- [ ] Release blocked until physical evidence confirms real TLS handshake,
      certificate verification and factory/OTA recoverability.
