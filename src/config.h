#ifndef CONFIG_H
#define CONFIG_H

#ifdef APP_VERSION
#define VERSION APP_VERSION // set by CMake from project(VERSION)
#else
#define VERSION "1.6.0"
#endif

#define CHANGELOG_TEXT "What's new in " VERSION ":\n"                                                  \
                       " - Qt 6 support (Qt 5.15 still works), CMake build, QCustomPlot 2.1.1\n"      \
                       " - Parser: scientific notation, '+' signs, 'Label: value', time-range filter\n" \
                       " - Logger: correct timestamps, aligned CSV columns, appends instead of overwriting\n" \
                       " - HEX/BIN views show raw bytes; UTF-8 split across reads decoded correctly\n" \
                       " - Serial: port chosen by name, unplugged device detected and closed\n"      \
                       " - Graph export to PNG/JPG/BMP/PDF; many crash fixes\n"

#define INTRO_TEXT ""                                                                           \
                   "Welcome to QtSerialMonitor, \n"                                             \
                   "Press F1 to activate \"What's this\" mode. In this mode, clicked widget \r" \
                   "will display an explanation about its function. \n"

#define RADIO_BUTTON_UPDATE_SERIAL_DEVICES_ON_INTERVAL 100
#define SERIAL_DEVICE_CHECK_TIMER_INTERVAL 500

#endif // CONFIG_H
