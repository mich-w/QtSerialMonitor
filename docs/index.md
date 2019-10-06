# QtSerialMonitor
Universal serial monitor with data plotting capabilities, based on [Qt](https://www.qt.io/). Designed with all sorts of [Arduino](https://www.arduino.cc/) projects in mind, as a handy tool for debugging and experimentation :) 

<img src="https://raw.githubusercontent.com/mich-w/QtSerialMonitor/master/docs/_screenshots/mainwindow.jpg">

Features:
- In/out serial data terminal with command history,
- UDP network protocol support,
- Advanced data plotter with multiple graphs support and basic data filtering - uses [QCustomPlot](https://www.qcustomplot.com/),
- Printer support, ability to save graph as image,
- Read/write ".txt" data logs,
- many more...

Enjoy !

<a href="https://github.com/mich-w/QtSerialMonitor/releases/download/v1.0/QtSerialMonitor_1.0_win_x64.zip" download>Download QtSerialMonitor_v1.0_win_x64</a> 

*Notes:*


*The app uses a custom-written parser, which searches the incoming message for plottable data in form of label-value set. At the moment, all labels and values **must be separated with a whitespace** in order to be recognized. Its possible to use separators like "=", ":" and "," - parser will be replace them with whitespace before processing. If no label is found, a generic name will be used i.e. "Graph 0".*

*Examples of supported formats:*

- *"Roll = 1.23 Pitch = 45.6"*
- *"Voltage: 1.23 (tabulator) Output: 4.56"*
- *"1.23 4.56" (Graph_0 and Graph_1)*
