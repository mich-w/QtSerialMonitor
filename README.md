# QtSerialMonitor
Universal serial / network terminal with data plotting capabilities, based on [Qt](https://www.qt.io/).

![mainwindow](/screenshots/mainwindow.png?raw=true "mainwindow")

Features:
- In/out serial data terminal with command history,
- UDP network protocol support,
- Advanced data plotter with multible graphs support and basic data filtering - uses [QCustomPlot](https://www.qcustomplot.com/),
- Printer support, ability to save graph as image,
- Read/write .txt data logs,
- many more...

<a href="https://github.com/mich-w/QtSerialMonitor/releases/download/v1.0/QtSerialMonitor_1.0_win_x64.zip" download>Download QtSerialMonitor_v1.0_win_x64</a>

The app uses a custom-written parser, which searches the incoming message for plottable data in form of label-value set. At the moment all labels and values **must be seperated with a whitespace** for the process to work properly. Its possible to use seperators like "=", ":" and "," - parser will be replace them with whitespace before processing. If no label is found, a generic name will be used i.e. "Graph 0". 

Examples of supported formats:

- "label_1 = 1.23 label_2 = 4.56" 
- "label_1 1.23 (tabulator) label_2 4.56" 
- "1.23 4.56" (Graph_0 and Graph_1)


