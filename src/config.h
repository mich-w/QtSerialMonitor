#ifndef CONFIG_H
#define CONFIG_H

#ifdef APP_VERSION
#define VERSION APP_VERSION // set by CMake from project(VERSION)
#else
#define VERSION "1.7.0"
#endif

#define CHANGELOG_TEXT "What's new in " VERSION ":\n"                                                    \
                       " - Values split across reads are no longer plotted in halves\n"                \
                       " - UDP datagrams without newline are kept separate\n"                          \
                       " - Files load in the background with a working Cancel button\n"                \
                       " - CSV loading: empty cells, quoted fields, ';' and tab delimiters\n"          \
                       " - Window and panel layout restored exactly between sessions\n"

#define INTRO_TEXT ""                                                                           \
                   "Welcome to QtSerialMonitor, \n"                                             \
                   "Press F1 to activate \"What's this\" mode. In this mode, clicked widget \r" \
                   "will display an explanation about its function. \n"

#define RADIO_BUTTON_UPDATE_SERIAL_DEVICES_ON_INTERVAL 100
#define SERIAL_DEVICE_CHECK_TIMER_INTERVAL 500

#endif // CONFIG_H
