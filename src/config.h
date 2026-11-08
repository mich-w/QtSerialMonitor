#ifndef CONFIG_H
#define CONFIG_H

#ifdef APP_VERSION
#define VERSION APP_VERSION // set by CMake from project(VERSION)
#else
#define VERSION "1.9.0"
#endif

#define CHANGELOG_TEXT "What's new in " VERSION ":\n"                                                    \
                       " - Pause the chart (Space) and scroll back through history with the bar below it\n" \
                       " - Min / Max / Mean / Count / Rate columns in the parsed data table\n"         \
                       " - Up/Down in the send box recalls previous commands\n"                         \
                       " - Macro buttons (+ to add, right-click to edit) and 'Repeat every N ms'\n"     \
                       " - Sent text goes to every open connection, whichever tab is selected\n"

#define INTRO_TEXT ""                                                                           \
                   "Welcome to QtSerialMonitor, \n"                                             \
                   "Press F1 to activate \"What's this\" mode. In this mode, clicked widget \r" \
                   "will display an explanation about its function. \n"

#define RADIO_BUTTON_UPDATE_SERIAL_DEVICES_ON_INTERVAL 100
#define SERIAL_DEVICE_CHECK_TIMER_INTERVAL 500

#endif // CONFIG_H
