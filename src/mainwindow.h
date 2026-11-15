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
#include "channelstats.h"
#include "commandhistory.h"
#include "csvexport.h"
#include "logtablemodel.h"
#include "orientationview.h"
#include "icons.h"
#include "settingsdialog.h"
#ifdef QTSM_HAS_QT_GRAPHS
#include "graphschartview.h"
#endif
#include "theme.h"
#include "macrobar.h"
#include "demotransport.h"
#include "portidentity.h"
#include "portscanner.h"
#include "receivepipeline.h"
#include "widgetsettings.h"

#include <QCloseEvent>
#include <QDateTime>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QKeyEvent>
#include <QList>
#include <QActionGroup>
#include <QPointer>
#include <QMainWindow>
#include <QFormLayout>
#include <QIntValidator>
#include <QGroupBox>
#include <QScreen>
#include <QToolBar>
#include <QToolButton>
#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QStandardPaths>
#include <QtDebug>
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
    void applyPreferences(const AppPreferences &newPreferences); // apply and save
    bool saveChartImage(QString filename);                        // PNG, JPG, BMP or PDF, by extension
    ~MainWindow();
public slots:
    void on_setSelectedLabels(QList<QString> *labels, bool customRules = true);
    void handleFileLoaded(const FileReader::Result &result);
    void closeEvent(QCloseEvent *event) override;
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
    void on_pushButtonSaveParserMemory_clicked();
    void on_pushButtonSend_clicked();
    void on_pushButtonSerialConnect_toggled(bool checked);
    void on_pushButtonSerialLogScrollown_clicked();
    void on_pushButtonSetDefaultLogPath_clicked();
    void on_pushButtonSetSelectedToGraph_clicked();
    void on_pushButtonTextLogToggle_toggled(bool checked);
    void on_pushButtonUDPConnect_toggled(bool checked);
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
    FileReader fileReader;
    InfoDialog infoDialog;
    Logger fileLogger;
    NetworkUDP networkUDP;
    Parser parser;
    QCPItemTracer *phaseTracer = nullptr;
    QHash<QString, int> missingCount; // per graph name: consecutive updates without new data
    int graphColorIndex = 0;
    QString controlCharactersVisibleConvert(QString text);
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
    AppPreferences preferences = AppPreferences::load();
    void openSettings();
    void applyOpenGl();
    void applyChartEngine();
    bool chartHasFocus() const; // QCustomPlot or the Qt Graphs chart
    QSize chartSize() const;     // of the chart on screen
    QString withLogExtension(QString fileName) const;
    static constexpr const char *autoLogFileName = "%DateTime%_Log"; // the logger fills in the date
    QString manualLogFileName;
    void updateChartRanges();
#ifdef QTSM_HAS_QT_GRAPHS
    QPointer<GraphsChartView> graphsChartView;
#endif
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
    void processIncomingLines(const QString &lines, const QList<long> &arrivalTimes);
    Ui::MainWindow *ui;
    void addLog(QString text, bool appendAsLine = false);
    void addLogBytes(QByteArray bytes, bool hexToBinary = false, bool appendAsLine = false);
    void clearGraphData(bool replot);
    void clearGraphs(bool replot);
    void createChart();
    void createChartTracer();
    void createTimers();
    static QString defaultLogPath();
    void exportArraysToCSV(QStringList labelList, QList<QList<double>> dataColums, QChar sep = ',');
    void exportTableLogToCSV(QTableView *table, QChar sep = ',');
    void loadFromRAM(bool loadText);
    void processChart(QStringList labelList, QList<double> numericDataList, QList<long> timeStampsList);
    void processLogTable(QList<long> timeTable, QStringList labelTable, QList<double> valueTable);
    void processLogWrite(QString rawLine, QStringList labelList, QList<double> dataList, QList<long> timeList);
    void processTable(QStringList labels, QList<double> values, QList<long> timeStamps);
    void clearParsedDataTable();
    void refreshParsedDataTable();
    bool parsedDataDirty = false;
    bool parsedDataRowsChanged = false;
    QElapsedTimer parsedDataResizeClock;
    QHash<QString, ChannelStats> channelStats;
    void saveToRAM(QStringList newlabelList, QList<double> newDataList, QList<long> newTimeList, bool saveText = false, QString text = "");
    void sendMessageLineEdit();
    bool sendText(const QString &text);
    void addToCommandHistory(const QString &text);
    void setRepeatSend(bool enabled);
    CommandHistory commandHistory;
    LogTableModel logTableModel;
    OrientationView *orientationView = nullptr;
    double orientationValues[3] = {0, 0, 0}; // roll, pitch, yaw as received
    bool orientationReceived = false;
    bool orientationDirty = false;
    void setupOrientationView();
    void setupTextViewOptions();
    void setupControlSection();
    void setupLoggingTabs();
    void setupChartOptions();
    QStringList customGraphLabels() const;
    QFrame *makeOptionsPopup(QToolButton *button, const QString &objectName);
    QHBoxLayout *chartBarLayout = nullptr; // Pause, history scroll bar and chart actions
    QCheckBox *checkBoxShowPointMarkers = nullptr;
    void updateOrientation(const QStringList &labels, const QList<double> &values);
    void refreshOrientationView();
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
    void keyPressEvent(QKeyEvent *event) override;
};

enum TAB_INDEX
{
    SERIAL_USB = 0,
    WIFI_UDP,
};

#endif // MAINWINDOW_H
