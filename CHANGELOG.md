# Changelog

All notable changes to this project are documented here.
Versions follow [Semantic Versioning](https://semver.org/).

## [Unreleased] – towards 2.0.0

## [1.8.0]

### Added
- Demo mode: a simulated device streaming sine waves, selectable as *(Demo) Simulated sine waves* or with `--demo` / `--demo-rate <hz>`.
- Command line options `--help` and `--version`.
- Auto-reconnect: an unplugged USB device is found again (by serial number, VID:PID or port name) and reopened.
- Arduino IDE 2 Serial Plotter format (`Sensor1:12,Sensor2:34`) and Teleplot format (`>temp:21.5`, `>temp:<ms>:21.5`).

### Fixed
- A second main window could show an empty device list.

## [1.7.0]

### Changed
- Received data is pushed from the serial port / UDP socket as it arrives instead of being polled by timers; the "processing delay" now only batches updates.
- Only complete lines are parsed and logged; the terminal still shows data immediately.
- Files are loaded and parsed on a background thread; Cancel works and the UI stays responsive.
- Window geometry and splitter layout are saved with Qt's native state; older settings are migrated automatically.
- Code restructured: `src/core` library (no widget dependencies), unit tests and a headless integration test run with `ctest` and in CI.

### Fixed
- Values split across two reads were plotted as two wrong values (`Temp 23` + `.5`).
- Consecutive UDP datagrams without a trailing newline were glued together.
- CSV loading shifted values to the wrong column after an empty cell.
- Re-entrancy while loading large files (the parser pumped the event loop).
- UDP port fields accepted values above 65535.

## [1.6.0]

### Added
- Qt 6 support; Qt 5.15 remains supported.
- CMake build system (replaces qmake), GitHub Actions CI for Linux, Windows and macOS.
- Graph export to PNG, JPG, BMP and PDF; "Save Graph As..." now works.
- Detection of unplugged serial devices – the port is closed and the error reported.
- Parser: scientific notation (`1e-3`), explicit `+` signs, `Label: value` style, ISO 8601 timestamps in reloaded logs.

### Changed
- QCustomPlot updated from 2.0.1 to 2.1.1 and moved to `third_party/`.
- Default log directory is `Documents/QtSerialMonitor/Logs`.
- Log timestamps are ISO 8601 (`yyyy-MM-ddTHH:mm:ss.zzzZ`), numbers use up to 10 significant digits.
- Labels written as `Voltage: 1.23` are now named `Voltage` (trailing colon stripped).

### Fixed
- Crashes: out-of-range serial port selection, external clock label at end of line, empty CSV header rewrite, tracer dangling pointers, "Set as clock label" without selection, undefined return in file time-range scan.
- Logger: minutes written as month, misaligned CSV columns, header rewrite corrupting files, TXT logs overwriting existing files, truncate option ignored.
- Parser split lines on `+`; time-range filter had no effect.
- Graphs/table rows skipped when removing stale labels; log table dropped first values and scattered values while sorting.
- HEX/BIN views corrupted non-UTF-8 bytes; BIN showed negative values; UDP truncated datagrams at NUL bytes.
- Device list refreshed every 500 ms when no devices were present.
- Settings for the log table toggle were not persisted.

## [1.53]
- Last release built with qmake / Qt 5.
