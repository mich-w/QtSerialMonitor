#ifndef CONFIG_H
#define CONFIG_H

#ifdef APP_VERSION
#define VERSION APP_VERSION // set by CMake from project(VERSION)
#else
#define VERSION "1.8.0"
#endif

#define CHANGELOG_TEXT "What's new in " VERSION ":\n"                                                    \
                       " - Demo mode: pick '(Demo) Simulated sine waves' or start with --demo\n"        \
                       " - Auto-reconnect when a USB device is unplugged and plugged back in\n"        \
                       " - Arduino Serial Plotter (Sensor1:12,Sensor2:34) and Teleplot (>temp:21.5) formats\n" \
                       " - Command line options: --demo, --demo-rate, --help, --version\n"

#define INTRO_TEXT ""                                                                           \
                   "Welcome to QtSerialMonitor, \n"                                             \
                   "Press F1 to activate \"What's this\" mode. In this mode, clicked widget \r" \
                   "will display an explanation about its function. \n"

#define RADIO_BUTTON_UPDATE_SERIAL_DEVICES_ON_INTERVAL 100
#define SERIAL_DEVICE_CHECK_TIMER_INTERVAL 500

#endif // CONFIG_H
