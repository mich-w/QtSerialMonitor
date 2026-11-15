cmake_minimum_required(VERSION 3.16)

project(QtSerialMonitor
    VERSION 1.9.0
    DESCRIPTION "Serial monitor with data plotting"
    HOMEPAGE_URL "https://github.com/mich-w/QtSerialMonitor"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Prefer Qt 6, fall back to Qt 5.15. Force one with -DQTSM_QT_MAJOR=5 (or 6).
set(QTSM_QT_MAJOR "" CACHE STRING "Qt major version to build against (5 or 6, empty = auto)")
if(QTSM_QT_MAJOR)
    set(QT_VERSION_MAJOR ${QTSM_QT_MAJOR})
else()
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
endif()
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets SerialPort PrintSupport Network)
set(QTSM_QT_VERSION ${Qt${QT_VERSION_MAJOR}_VERSION})
if(QTSM_QT_VERSION VERSION_LESS 5.15)
    message(FATAL_ERROR "Qt 5.15 or newer is required (found ${QTSM_QT_VERSION})")
endif()
message(STATUS "Building with Qt ${QTSM_QT_VERSION}")

# ---------------------------------------------------------------------------
# Third party: QCustomPlot
# ---------------------------------------------------------------------------
add_library(qcustomplot STATIC
    third_party/qcustomplot/qcustomplot.cpp
    third_party/qcustomplot/qcustomplot.h)
target_include_directories(qcustomplot PUBLIC third_party/qcustomplot)

# Optional OpenGL rendering of the chart (Settings -> Performance, off by default).
# Qt 6 moved the needed classes into the separate OpenGL module; on Qt 5 they are part of QtGui.
option(QTSM_OPENGL "Build QCustomPlot with optional OpenGL rendering" ON)
if(QTSM_OPENGL)
    if(QT_VERSION_MAJOR GREATER_EQUAL 6)
        find_package(Qt6 QUIET COMPONENTS OpenGL)
        set(QTSM_OPENGL_AVAILABLE ${Qt6OpenGL_FOUND})
    else()
        find_package(OpenGL QUIET) # Qt 5's QtGui does not pass libGL on to the linker
        set(QTSM_OPENGL_AVAILABLE ${OpenGL_FOUND})
    endif()
    if(QTSM_OPENGL_AVAILABLE)
        # PUBLIC: the define changes QCustomPlot's class layout, so every user of the header needs it.
        target_compile_definitions(qcustomplot PUBLIC QCUSTOMPLOT_USE_OPENGL)
        if(QT_VERSION_MAJOR GREATER_EQUAL 6)
            target_link_libraries(qcustomplot PUBLIC Qt6::OpenGL)
        else()
            target_link_libraries(qcustomplot PUBLIC OpenGL::GL)
        endif()
        message(STATUS "OpenGL chart rendering: available")
    else()
        message(STATUS "OpenGL chart rendering: not available (Qt OpenGL module not found)")
    endif()
endif()
target_link_libraries(qcustomplot PUBLIC Qt::Widgets Qt::PrintSupport)
set_target_properties(qcustomplot PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ---------------------------------------------------------------------------
# Core library: parsing, logging, transports - no widget dependencies
# ---------------------------------------------------------------------------
add_library(qtsm_core STATIC
    src/core/channelstats.h
    src/core/commandhistory.cpp src/core/commandhistory.h
    src/core/csvexport.cpp    src/core/csvexport.h
    src/core/demotransport.cpp src/core/demotransport.h
    src/core/filereader.cpp   src/core/filereader.h
    src/core/logger.cpp       src/core/logger.h
    src/core/logtablemodel.cpp src/core/logtablemodel.h
    src/core/networkudp.cpp   src/core/networkudp.h
    src/core/parser.cpp       src/core/parser.h
    src/core/portidentity.cpp src/core/portidentity.h
    src/core/portscanner.cpp  src/core/portscanner.h
    src/core/receivepipeline.cpp src/core/receivepipeline.h
    src/core/serial.cpp       src/core/serial.h
    src/core/transport.h
    src/core/utf8decoder.h)
target_include_directories(qtsm_core PUBLIC src/core)
target_link_libraries(qtsm_core PUBLIC Qt::Core Qt::Network Qt::SerialPort)

# ---------------------------------------------------------------------------
# Application
# ---------------------------------------------------------------------------
# GUI code as a library so integration tests can drive MainWindow.
add_library(qtsm_gui STATIC
    src/codeeditor.cpp       src/codeeditor.h
    src/config.h
    src/icons.cpp            src/icons.h
    src/infodialog.cpp       src/infodialog.h       src/infodialog.ui
    src/resources.qrc
    src/macrobar.cpp         src/macrobar.h
    src/mainwindow.cpp       src/mainwindow.h       src/mainwindow.ui
    src/orientationview.cpp  src/orientationview.h
    src/settingsdialog.cpp   src/settingsdialog.h
    src/theme.cpp            src/theme.h
    src/widgetsettings.cpp   src/widgetsettings.h)

# Generated ui_*.h files include promoted widget headers (e.g. codeeditor.h).
target_include_directories(qtsm_gui PUBLIC src)

target_compile_definitions(qtsm_gui PUBLIC
    APP_VERSION="${PROJECT_VERSION}"
    QT_DEPRECATED_WARNINGS)

target_link_libraries(qtsm_gui PUBLIC
    qtsm_core
    qcustomplot
    Qt::Widgets
    Qt::SerialPort
    Qt::PrintSupport
    Qt::Network)

# Optional experimental chart engine drawn with Qt Graphs (Settings -> Performance). Needs Qt 6.8+.
option(QTSM_QT_GRAPHS "Build the optional Qt Graphs chart engine" ON)
if(QTSM_QT_GRAPHS AND QT_VERSION_MAJOR GREATER_EQUAL 6 AND QTSM_QT_VERSION VERSION_GREATER_EQUAL 6.8)
    find_package(Qt6 QUIET COMPONENTS Graphs Quick QuickWidgets Qml)
endif()
if(Qt6Graphs_FOUND AND Qt6QuickWidgets_FOUND)
    target_sources(qtsm_gui PRIVATE src/graphschartview.cpp src/graphschartview.h)
    qt_add_resources(qtsm_gui graphs_qml PREFIX /qml BASE src/qml FILES src/qml/GraphsChart.qml)
    target_link_libraries(qtsm_gui PUBLIC Qt6::Graphs Qt6::Quick Qt6::QuickWidgets Qt6::Qml)
    target_compile_definitions(qtsm_gui PUBLIC QTSM_HAS_QT_GRAPHS)
    message(STATUS "Qt Graphs chart engine: available")
else()
    message(STATUS "Qt Graphs chart engine: not available (needs Qt 6.8+ with Graphs, Quick and QuickWidgets)")
endif()

set(APP_SOURCES src/main.cpp)

if(WIN32)
    list(APPEND APP_SOURCES src/QtSerialMonitor.rc)
endif()

add_executable(QtSerialMonitor ${APP_SOURCES})

target_link_libraries(QtSerialMonitor PRIVATE qtsm_gui)

foreach(target qtsm_core qtsm_gui QtSerialMonitor)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()
endforeach()

set_target_properties(QtSerialMonitor PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_BUNDLE_NAME QtSerialMonitor
    MACOSX_BUNDLE_GUI_IDENTIFIER io.github.mich-w.QtSerialMonitor
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})

# Some Linux file managers refuse to launch PIE executables by double-click.
if(UNIX AND NOT APPLE)
    option(QTSM_NO_PIE "Link as a non-PIE executable" ON)
    if(QTSM_NO_PIE)
        set_target_properties(QtSerialMonitor PROPERTIES POSITION_INDEPENDENT_CODE OFF)
        target_link_options(QtSerialMonitor PRIVATE -no-pie)
    endif()
endif()

# ---------------------------------------------------------------------------
# Tests (ctest)
# ---------------------------------------------------------------------------
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

include(GNUInstallDirs)
install(TARGETS QtSerialMonitor
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
