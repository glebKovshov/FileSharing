# LAN protocol v2

Discovery uses IPv4 UDP 12345 on all active network interfaces. Queries and
announcements repeat every two seconds; offers expire after seven seconds.
Datagrams are compact UTF-8 JSON (at most 4096 bytes) with `version: 2`, `type`,
and a random 32-character hexadecimal instance `id`.

- `query`: asks senders to reply directly to the query's source address/port.
- `offer`: includes `offer` (fresh random identifier), `device`, `name`, `size`,
  and `port` (the sender's dynamically assigned TCP listener).
- `withdraw`: removes an offer from receiver lists.

Sending starts only after a local user selects a readable file and explicitly
starts sharing. Receiving starts only after selecting a discovered offer and
confirming its name, size, sender and destination folder. One transfer is active
per application instance. The sender stops sharing after the GUI transfer ends;
users can explicitly share the same file again.

## TCP framing

Each control message is a four-byte unsigned **big-endian** JSON byte length
followed by that many UTF-8 bytes. Length must be 1–16384. Headers can span TCP
reads; several protocol parts can arrive in one read. File sizes are nonnegative
integers, limited to 8 PiB and encoded exactly within JSON's safe integer range.

1. Receiver sends `pull` with the confirmed `offer` identifier.
2. Sender sends `file` with `offer`, UTF-8 basename `name`, and `size`.
3. Sender streams **exactly `size` bytes** in bounded 64 KiB chunks.
4. Sender sends a `digest` control frame with the SHA-256 hex digest of those bytes.
5. Receiver checks the metadata against the confirmed offer, checks disk space,
   and writes a hidden temporary file in the destination directory. Only after
   the exact-length payload and digest pass validation is the temporary file
   renamed to its final name. Existing files are never overwritten; a numeric
   suffix is added, with rename failures checked for races.
6. Receiver sends `complete` with the matching digest after the rename. Sender
   reports success only after receiving that acknowledgement. The receiver
   socket stays alive until the acknowledgement buffer drains.

Buffers and peer lists are bounded. Every connection has a 45-second inactivity
timeout, including the initial handshake and final acknowledgement. Cancellation,
invalid framing, stale metadata, short reads, write failures, hash mismatches and
connection errors abort the session and remove its temporary file. Partial
receives are never reported as complete. The UI remains responsive through the
Qt event loop; it uses no blocking network calls or busy-wait threads.

Names are Unicode NFC, at most 240 UTF-8 bytes, and reject directory separators,
control characters and Windows-reserved names. Local paths are never transmitted.
Legacy unstructured datagrams and incompatible protocol versions are ignored.
Old v1 executables cannot exchange files with v2; update both devices.

## Integrity and trust

SHA-256 detects corruption; this LAN protocol does not authenticate devices or
provide encryption. Use it on trusted local networks. If acknowledgement is lost
after a successful rename, the sender can show an error even though the receiver
saved the file; a retry creates a separate copy instead of overwriting it.
Automatic resume is intentionally not implemented: interrupted transfers restart
from zero after explicit user action.
