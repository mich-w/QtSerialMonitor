#ifndef CONFIG_H
#define CONFIG_H

#ifdef APP_VERSION
#define VERSION APP_VERSION // set by CMake from project(VERSION)
#else
#define VERSION "2.0.0"
#endif

#define CHANGELOG_TEXT "What's new in " VERSION ":\n"                                                    \
                       " - Light and dark themes - use the button in the top-right corner or View > Theme\n" \
                       " - Refreshed look and a toolbar for the most used actions\n"                   \
                       " - Demo mode, auto-reconnect, Arduino Serial Plotter and Teleplot formats\n"   \
                       " - Chart pause and history, per-channel statistics\n"                          \
                       " - Command history (Up/Down), macro buttons, periodic send\n"                  \
                       " - Much faster with high data rates; Tools > Settings for refresh rate and line quality\n" \
                       " - Qt 6 support and many fixes - see CHANGELOG.md for the full list\n"

#define INTRO_TEXT ""                                                                           \
                   "Welcome to QtSerialMonitor, \n"                                             \
                   "Press F1 to activate \"What's this\" mode. In this mode, clicked widget \r" \
                   "will display an explanation about its function. \n"

#define RADIO_BUTTON_UPDATE_SERIAL_DEVICES_ON_INTERVAL 100
#define SERIAL_DEVICE_CHECK_TIMER_INTERVAL 500

#endif // CONFIG_H
