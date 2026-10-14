# Verification

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

`core_tests` uses actual loopback TCP/UDP sockets and checks UTF-8 fragmentation,
malformed/legacy discovery, unsafe paths, exact byte equality for empty files,
one-byte files, 64 KiB boundaries and multi-megabyte payloads, bad checksums,
extra data, changed metadata, disconnects, cancellation, timeouts, repeated
transfers and collision-safe filenames. `ui_tests` checks first-launch state,
explicit sharing, stopping, receiver controls and readable error display.

To render both native UI screens without a display:

```sh
QT_QPA_PLATFORM=offscreen FS_SCREENSHOTS="$PWD/artifacts/screenshots" ./build/ui_tests
QT_QPA_PLATFORM=offscreen ./build/FileSharing --smoke-test
```

GitHub Actions builds and runs the tests on Linux x64/ARM64, Windows x64 and a
universal macOS Intel/Apple Silicon binary. Installers are uploaded as workflow
artifacts. Packaged Windows and AppImage binaries also run a headless startup
check to detect missing runtime dependencies.

Before distributing a release, use two physical computers to check discovery
across Wi-Fi/Ethernet, OS firewall permissions, cancellation during a large
transfer, recovery after unplugging the network, a read-only destination, and
saving Unicode filenames in non-ASCII paths. Loopback tests do not validate
router broadcast filtering or OS permission dialogs. Validate installation and
uninstallation on each supported OS; Apple signing/notarization requires the
maintainer's certificate and credentials.
