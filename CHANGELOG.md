# Changelog

All notable changes to this project are documented here.
Versions follow [Semantic Versioning](https://semver.org/).

## [Unreleased] – towards 2.0.0

### Added
- Changelog tab in Help → Application Info; Help → What's new opens it.
- Custom baud rates: type any rate (e.g. 1152000, 2000000); it is kept between sessions (#5).
- "Mark data points" option in the chart options (off by default) - plain lines otherwise (#11).
- Light and dark themes, or follow the system setting; theme button in the menu bar and View → Theme.
- Main toolbar: Connect, Pause, Clear, Save log, Export chart.
- Tools menu with *Demo device*; the demo device is listed among serial devices only while it runs.

### Fixed
- Exporting large data to CSV took hours: 200 000 rows now take well under a second (#7).
- Points piled up on the same time stamp at high data rates. Each line now gets the time it arrived, and lines that arrive together are spread over the time since the previous chunk (#8).
- Glitches with fast data: values split across two reads were plotted in halves. Fixed by parsing only complete lines; a test feeds the reporter's sketch output split at every possible point (#9).
- "Plot only" split labels with spaces ("Graph 0" became "Graph || 0") and matched parts of labels ("Graph 1" also showed "Graph 10"). Labels are now separated by `||` or commas and matched exactly (#10).
- The "Save" button on the Parser memory tab did nothing: its handler was named after a renamed widget and never connected. It also stored one label per graph instead of per value and time stamps in seconds instead of milliseconds.
- Every received line was displayed, plotted and logged twice, because the serial port and the demo device share a receive pipeline and its outputs were connected once per transport.
- Up/Down in the send box walked the history in the wrong direction; Up now recalls the most recent command, like a terminal.
- CSV and parsed-data logs kept only the last line of each received chunk; every line now gets its own row, time-stamped with when it arrived.
- Load file read the file as CSV or text depending on the logging format setting; it now goes by the file extension.
- Qt Graphs chart: Clear, show/hide graphs and other manual changes were not shown until new data arrived (or at all while paused).
- Chart keyboard shortcuts (Space/P, X, Y, T, L) did nothing on the Qt Graphs chart.

- Settings window (Tools → Settings…, Ctrl+,): theme, chart refresh rate (30/60 fps) and line rendering (Smooth/Fast).
- 3D orientation view (View → Change visualisation → 3D Orientation, Ctrl+9): a board rotated by the latest roll / pitch / yaw values, with configurable labels and degrees or radians. While it is shown, the demo device also streams roll / pitch / yaw. Drag to rotate the camera, mouse wheel to zoom, double-click to reset.

- The "File storage" tab is split into "Logging" (with a full-width log folder field, Browse, Default and Open folder - auto-logging writes there) and "Load file".
- Chart: the three rows of options under the chart are merged into the Pause row (Auto-scroll, time window, Clear plot, Parser data) and an Options popup, giving the chart more height.
- Text view: the crowded, horizontally scrolling options area is replaced by one row (Pause text, Auto-scroll, Newest, Clear, highlight field) and an Options popup with the remaining settings, grouped and labelled.

### Removed
- The unused Qt 3D Studio orientation prototype and its resource files.

### Performance
Measured with the demo device at 1000 lines/s (headless, Qt 6): the app used to keep up with ~200 lines/s
at full CPU; it now processes ~650 lines/s with smooth lines and ~950 lines/s at 38% CPU with fast lines.
- The chart is redrawn at a fixed rate (30 or 60 fps) instead of after every batch of data.
- Serial ports are enumerated on a background thread (~14 ms per scan used to freeze the UI twice a second).
- The log table uses a lightweight model; auto-scroll happens once per refresh and only while visible.
- The statistics table is updated once per refresh instead of per batch.
- Optional fast line rendering makes a chart redraw ~12× cheaper.
- Experimental OpenGL chart rendering (Settings → Performance, off by default). Measured on a Wayland desktop at
  1000 lines/s: 6.8 ms per frame with smooth lines vs 14.3 ms in software; software Fast lines remain the
  cheapest at 1.4 ms. Falls back to normal drawing if OpenGL cannot be started.
- Experimental Qt Graphs chart engine (Settings → Performance → Chart engine, Qt 6.8+ builds): the chart is
  drawn on the GPU with Qt Quick while QCustomPlot keeps the data. Zoom, pan, rectangle zoom, fit and a context
  menu work; the tracer does not yet. Measured with the demo at 1000 lines/s: ~20-24% CPU vs ~60% with
  QCustomPlot.
- The demo device now sends every sample that is due, so its rate stays exact when the app is busy.

### Changed
- Refreshed look: new palettes, rounded controls, accent colour, cleaner tabs, tables and scroll bars.
- Chart follows the theme; series use a curated palette and smooth lines instead of random marker shapes.
- README: new screenshots and feature list. Releases are built by CI and attached to GitHub releases.

## [1.9.0]

### Added
- Chart pause (button, Space or P) – data keeps being recorded – and a history scroll bar under the chart.
- Per-channel statistics in the parsed data table: Min, Max, Mean, Count, Rate [Hz]; "Reset statistics" in its context menu.
- Shell-like command history with Up/Down, most recent first, no duplicates.
- Macro buttons for frequently sent commands.
- Periodic send ("Repeat every N ms").

### Changed
- Sent text goes to every open connection instead of depending on the selected control tab.

### Fixed
- Log table merged lines from the same processing batch into one row, overwriting values.

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
