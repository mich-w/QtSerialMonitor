#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "config.h"
#include "filereader.h"
#include "logger.h"
#include "networkudp.h"
#include "parser.h"
#include "qcustomplot.h"
#include "serial.h"
#include "infodialog.h"
#include "highlighter.h"
#include "channelstats.h"
#include "commandhistory.h"
#include "logtablemodel.h"
#include "icons.h"
#include "theme.h"
#include "macrobar.h"
#include "demotransport.h"
#include "portidentity.h"
#include "portscanner.h"
#include "receivepipeline.h"
#include "widgetsettings.h"

#include <QCloseEvent>
#include <QCompleter>
#include <QDateTime>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QKeyEvent>
#include <QList>
#include <QActionGroup>
#include <QMainWindow>
#include <QToolBar>
#include <QToolButton>
#include <QMessageBox>
#include <QMetaType>
#include <QPaintDevice>
#include <QRandomGenerator>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QStandardPaths>
#include <QtDebug>
#include <QTextEdit>
#include <QTextStream>
#include <QTime>
#include <QTimer>

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void startDemo(int rateHz = DemoTransport::DefaultRateHz); // select the simulated device and connect
    ~MainWindow();
public slots:
    void on_setSelectedLabels(QList<QString> *labels, bool customRules = true);
    void handleFileLoaded(const FileReader::Result &result);
    void closeEvent(QCloseEvent *event);
private slots:
    void on_aboutToQuitSlot();
    void on_action3D_orientation_triggered();
    void on_action50_50_view_triggered();
    void on_actionAbout_Qt_triggered();
    void on_actionFull_chart_triggered();
    void on_actionFull_parser_data_triggered();
    void on_actionFull_text_view_triggered();
    void on_actionHide_parser_data_triggered();
    void on_actionImage_triggered();
    void on_actionInfo_triggered();
    void on_actionPlotter_triggered();
    void on_actionPrint_Graph_triggered();
    void on_actionPrint_log_triggered();
    void on_actionQuit_triggered();
    void on_actionSave_As_triggered();
    void on_actionSave_graph_as_triggered();
    void on_actionShow_parser_data_triggered();
    void on_actionto_csv_triggered();
    void on_actionWhat_s_new_triggered();
    void on_actionWhats_this_triggered();
    void on_chartBeforeReplotSlot();
    void on_chartContextMenuRequest(QPoint pos);
    void on_chartMouseDoubleClickHandler(QMouseEvent *event);
    void on_chartMouseMoveHandler(QMouseEvent *event);
    void on_chartMousePressHandler(QMouseEvent *event);
    void on_chartSelectionChanged();
    void on_checkBoxAutoLogging_toggled(bool checked);
    void on_checkBoxAutoRefresh_toggled(bool checked);
    void on_checkBoxAutoSaveBuffer_toggled(bool checked);
    void on_checkBoxAutoTrack_toggled(bool checked);
    void on_checkBoxEnableTracer_toggled(bool checked);
    void on_checkBoxScrollLogEnableSorting_toggled(bool checked);
    void on_checkBoxShowLegend_toggled(bool checked);
    void on_checkBoxWrapText_toggled(bool checked);
    void on_clearGraphSelection();
    void on_comboBoxAddTextMode_currentIndexChanged(int index);
    void on_comboBoxClockSource_currentIndexChanged(int index);
    void on_comboBoxFormat_currentIndexChanged(int index);
    void on_comboBoxGraphDisplayMode_currentIndexChanged(int index);
    void on_comboBoxLogFormat_currentIndexChanged(int index);
    void on_comboBoxLoggingMode_currentIndexChanged(int index);
    void on_comboBoxSendReturnPressedSlot();
    void on_comboBoxSerialReadMode_currentIndexChanged(int index);
    void on_comboBoxTracerStyle_currentTextChanged(const QString &arg1);
    void on_comboBoxUDPReceiveMode_currentTextChanged(const QString &arg1);
    void on_comboBoxUDPSendMode_currentTextChanged(const QString &arg1);
    void on_hideAllGraphs();
    void on_hideSelectedGraph();
    void on_highlighLog(QString searchString);
    void on_lineEditCustomParsingRules_editingFinished();
    void on_lineEditExternalClockLabel_editingFinished();
    void on_lineEditHighlight_returnPressed();
    void on_lineEditLoadFilePath_customContextMenuRequested(const QPoint &pos);
    void on_lineEditLoadFilePath_textChanged(const QString &arg1);
    void on_lineEditSaveFileName_editingFinished();
    void on_printIntroChangelog();
    void on_printLog(QPrinter *printer);
    void on_printPlot(QPrinter *printer);
    void on_pushButtonAddDateTime_clicked();
    void on_pushButtonClear_clicked();
    void on_pushButtonClearAll_clicked();
    void on_pushButtonClearGraphs_clicked();
    void on_pushButtonClearHistory_clicked();
    void on_pushButtonClearLogTable_clicked();
    void on_pushButtonClearTable_clicked();
    void on_pushButtonEnablePlot_toggled(bool checked);
    void on_pushButtonEnableTableLog_toggled(bool checked);
    void on_pushButtonExportLogTableToCSV_clicked();
    void on_pushButtonLoadFile_clicked();
    void on_pushButtonLoadPath_clicked();
    void on_pushButtonLoadRAMBuffer_clicked();
    void on_pushButtonLogging_toggled(bool checked);
    void on_pushButtonLogPath_clicked();
    void on_pushButtonRAMClear_clicked();
    void on_pushButtonRefresh_clicked();
    void on_pushButtonSaveRAMBuffer_clicked();
    void on_pushButtonSend_clicked();
    void on_pushButtonSerialConnect_toggled(bool checked);
    void on_pushButtonSerialLogScrollown_clicked();
    void on_pushButtonSetDefaultLogPath_clicked();
    void on_pushButtonSetSelectedToGraph_clicked();
    void on_pushButtonTextLogToggle_toggled(bool checked);
    void on_pushButtonUDPConnect_toggled(bool checked);
    void on_radioButtonScrollToButtom_clicked();
    void on_showAllGraphs();
    void on_showSelectedGraph();
    void on_showSelectedGraphExclusively();
    void on_spinBoxMaxGraphs_valueChanged(int arg1);
    void on_spinBoxMaxLines_valueChanged(int arg1);
    void on_spinBoxProcessingDelay_valueChanged(int arg1);
    void on_splitterGraphTable_splitterMoved(int pos, int index);
    void on_tableWidgetParsedData_customContextMenuRequested(const QPoint &pos);
    void on_toolButtonAdvancedGraphMenu_clicked();
    void on_toolButtonHideTable_clicked();
    void on_tracerShowPointValue(QMouseEvent *event);
    void on_updateSerialDeviceList();

private:
    // QCompleter *completer;
    FileReader fileReader;
    Highlighter *highlighter;
    InfoDialog infoDialog;
    Logger fileLogger;
    NetworkUDP networkUDP;
    Parser parser;
    QCPItemTracer *phaseTracer = nullptr;
    QHash<QString, int> missingCount; // per graph name: consecutive updates without new data
    int graphColorIndex = 0;
    QString controlCharactersVisibleConvert(QString text);
    QStringList lineEditCustomParsingRulesAutoCompleteWordList;
    QTimer *radioButtonTimer;
    QTimer *serialDeviceCheckTimer;
    Serial serial;
    DemoTransport demo;
    QPushButton *pushButtonPauseChart = nullptr;
    QScrollBar *scrollBarChartHistory = nullptr;
    bool chartPaused = false;
    bool updatingHistoryScrollBar = false;
    void setChartPaused(bool paused);
    void applyTheme();
    void setupThemeControls();
    void updateThemeButton();
    void setupToolBar();
    void updateToolBar();
    QAction *actionConnect = nullptr;
    QAction *actionDemoDevice = nullptr;
    bool demoListed = false; // demo device shown in the device list only while used
    void setupToolsMenu();
    QAction *actionPause = nullptr;
    QAction *actionClear = nullptr;
    QAction *actionSaveLog = nullptr;
    QAction *actionExportImage = nullptr;
    QActionGroup *themeActions = nullptr;
    QToolButton *themeButton = nullptr;
    void styleGraph(QCPGraph *graph);
    QColor legendTextColor(bool graphVisible) const;
    void scrollChartHistory(int valueMs);
    void updateHistoryScrollBar();
    PortIdentity connectedPort; // remembered for auto-reconnect
    QTimer reconnectTimer;
    PortScanner portScanner;
    QList<PortIdentity> knownPorts; // result of the last background scan
    void handlePortScan(const QList<PortIdentity> &ports);
    void refreshDeviceList();
    QTimer chartRefreshTimer;
    static constexpr int DefaultChartRefreshRate = 30; // frames per second
    void requestChartRefresh();
    void refreshChart();
    void setChartRefreshRate(int framesPerSecond);
    bool waitingForReconnect = false;
    void handleSerialConnectionLost(const QString &reason);
    void tryReconnect();
    bool openSerialPort(const QString &portName);
    void stopReconnecting();
    int demoRate = DemoTransport::DefaultRateHz;
    static constexpr const char *demoPortId = "demo:";
    ReceivePipeline serialPipeline;
    ReceivePipeline udpPipeline;
    void connectTransport(Transport *transport, ReceivePipeline *pipeline);
    void displayIncoming(const QByteArray &bytes, const QString &text);
    void processIncomingLines(const QString &lines);
    Ui::MainWindow *ui;
    void addLog(QString text, bool appendAsLine = false);
    void addLogBytes(QByteArray bytes, bool hexToBinary = false, bool appendAsLine = false);
    void chartPrintPreview();
    void clearGraphData(bool replot);
    void clearGraphs(bool replot);
    void createChart();
    void createChartTracer();
    void createTimers();
    static QString defaultLogPath();
    void exportArraysToCSV(QStringList labelList, QList<QList<double>> dataColums, QChar sep = ',');
    void exportTableLogToCSV(QTableView *table, QChar sep = ',');
    void loadFromRAM(bool loadText);
    void menageAutoCompleteList(QString input);
    void processChart(QStringList labelList, QList<double> numericDataList, QList<long> timeStampsList);
    void processLogTable(QList<long> timeTable, QStringList labelTable, QList<double> valueTable);
    void processLogWrite(QString rawLine, QStringList labelList, QList<double> dataList, QList<long> timeList);
    void processTable(QStringList labels, QList<double> values, QList<long> timeStamps);
    void clearParsedDataTable();
    QHash<QString, ChannelStats> channelStats;
    void saveToRAM(QStringList newlabelList, QList<double> newDataList, QList<long> newTimeList, bool saveText = false, QString text = "");
    void sendMessageLineEdit();
    bool sendText(const QString &text);
    void addToCommandHistory(const QString &text);
    void setRepeatSend(bool enabled);
    CommandHistory commandHistory;
    LogTableModel logTableModel;
    bool logTableScrollPending = false;
    MacroBar *macroBar = nullptr;
    QCheckBox *checkBoxRepeatSend = nullptr;
    QSpinBox *spinBoxRepeatInterval = nullptr;
    QTimer repeatSendTimer;
    QString repeatSendText;
    void sendSerial(QString message);
    void sendUDPDatagram(QString message);
    void settingsLoadAll();
    void settingsSaveAll();
    void setupGUI();
    void setupTable();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event);
};

enum TAB_INDEX
{
    SERIAL_USB = 0,
    WIFI_UDP,
};

#endif // MAINWINDOW_H
