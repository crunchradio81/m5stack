# P8 network field acceptance — NOT TESTED on original K132

- [ ] Confirm original stock K132, no PSRAM/Grove hardware.
- [ ] Verify 15 SD app manifests and factory/OTA partition compatibility.
- [ ] Retain original firmware and SD backups before any flash erase.
- [ ] Boot NETTOOLS and Escape/backtick returns to factory suite.
- [ ] Absent/corrupt config refuses Wi-Fi connection; password never appears
      in UI, logs or repository.
- [ ] Wi-Fi joins an owner-controlled 2.4GHz network; times out after
      12 seconds on a bad password, disconnects on user command.
- [ ] IPv4 DNS returns expected address or visible failure.
- [ ] Real ICMP echo to known LAN peer reports valid round-trip/timeout;
      repeated requests do not leak ping tasks or heap.
- [ ] NTP shows correct UTC date/time; offline case has honest error.
- [ ] HTTP fetch from an owner-controlled local server reads safely up to
      configured bounds, rejects non-2xx/oversize responses and never
      silently disables TLS verification (only HTTP is implemented).
- [ ] mDNS discovers a known _http._tcp service on local network.
- [ ] MQTT subscribes to a non-sensitive test topic on a local broker,
      receives a message and disconnects cleanly.
- [ ] Two owner-controlled Cardputers or one UDP test peer exchange
      72-character ASCII LAN chat payloads with malformed packets rejected.
- [ ] RSS title extraction from a small test HTTP feed works, with
      malformed/oversize data causing an error not a crash.
- [ ] Repeated connect/disconnect and app switches run without heap leaks,
      freezes or secret disclosure.
- [ ] Verify physical keyboard S/B/Enter/Fn arrows and SD removal behavior.
- [ ] Record source commit, Actions run, test LAN setup, UART evidence,
      packet capture on owner network, screenshots and factory boot proof.

The entire firmware remains EXPERIMENTAL until real original-K132 tests pass.
