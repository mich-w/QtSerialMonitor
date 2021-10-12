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

#include <QCloseEvent>
#include <QCompleter>
#include <QDateTime>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QKeyEvent>
#include <QList>
#include <QMainWindow>
#include <QMessageBox>
#include <QMetaType>
#include <QPaintDevice>
#include <QRandomGenerator>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
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
    ~MainWindow();
public slots:
    void on_processLoadedFile(QString *text);
    void on_processLoadedFileLine(QString *line, int *progressPercent);
    void on_setSelectedLabels(QList<QString> *labels, bool customRules = true);
    void on_updateProgressBar(float *percent);
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
    void on_comboBoxTracerStyle_currentIndexChanged(const QString &arg1);
    void on_comboBoxUDPReceiveMode_currentIndexChanged(const QString &arg1);
    void on_comboBoxUDPSendMode_currentIndexChanged(const QString &arg1);
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
    void on_processSerial();
    void on_processUDP();
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
    QCPItemTracer *phaseTracer;
    QList<int> missingCount;
    QString controlCharactersVisibleConvert(QString text);
    QStringList lineEditCustomParsingRulesAutoCompleteWordList;
    QTimer *radioButtonTimer;
    QTimer *serialDeviceCheckTimer;
    QTimer *serialStringProcessingTimer;
    QTimer *udpStringProcessingTimer;
    Serial serial;
    Ui::MainWindow *ui;
    void addLog(QString text, bool appendAsLine = false);
    void addLogBytes(QByteArray bytes, bool hexToBinary = false, bool appendAsLine = false);
    void chartPrintPreview();
    void clearGraphData(bool replot);
    void clearGraphs(bool replot);
    void createChart();
    void createChartTracer();
    void createTimers();
    void exportTableLogToCSV(QTableView *table, QChar sep = ',');
    void loadFromRAM(bool loadText);
    void menageAutoCompleteList(QString input);
    void processChart(QStringList labelList, QList<double> numericDataList, QList<long> timeStampsList);
    void processLogTable(QList<long> timeTable, QStringList labelTable, QList<double> valueTable);
    void processLogWrite(QString rawLine, QStringList labelList, QList<double> dataList, QList<long> timeList);
    void processTable(QStringList labels, QList<double> values);
    void saveToRAM(QStringList newlabelList, QList<double> newDataList, QList<long> newTimeList, bool saveText = false, QString text = "");
    void sendMessageKeyEvent(QKeyEvent *event);
    void sendMessageLineEdit(int mode);
    void sendSerial(QString message);
    void sendUDPDatagram(QString message);
    void settingsLoadAll();
    void settingsSaveAll();
    void setupGUI();
    void setupTable();
    void exportArraysToCSV(QStringList labelList, QList<QList<double>> dataColums, QChar sep = ',');

protected:
    void keyPressEvent(QKeyEvent *event);
};

enum TAB_INDEX
{
    SERIAL_USB = 0,
    WIFI_UDP,
};

#endif // MAINWINDOW_H
