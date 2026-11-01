# Changelog

All notable changes to this project are documented here.
Versions follow [Semantic Versioning](https://semver.org/).

## [Unreleased] – towards 2.0.0

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
