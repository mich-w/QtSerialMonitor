#ifndef CONFIG_H
#define CONFIG_H

#ifdef APP_VERSION
#define VERSION APP_VERSION // set by CMake from project(VERSION)
#else
#define VERSION "1.53"
#endif

#define CHANGELOG_TEXT "" //"Changelog - version " VERSION ": \n"

#define INTRO_TEXT ""                                                                           \
                   "Welcome to QtSerialMonitor, \n"                                             \
                   "Press F1 to activate \"What's this\" mode. In this mode, clicked widget \r" \
                   "will display an explanation about its function. \n"

#define RADIO_BUTTON_UPDATE_SERIAL_DEVICES_ON_INTERVAL 100
#define SERIAL_DEVICE_CHECK_TIMER_INTERVAL 500

#endif // CONFIG_H
