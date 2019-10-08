#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "config.h"
#include "filereader.h"
#include "logger.h"
#include "networkudp.h"
#include "parser.h"
#include "qcustomplot.h"
#include "serial.h"

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

//#include <q3dsruntimeglobal.h>
//#include <Q3DSWidget>
//#include <Q3DSPresentation>

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
    void processLoadedFile(QString *text);
    void processLoadedFileLine(QString *line, int *progressPercent);
    void setSelectedLabels(QList<QString> *labels, bool customRules = true);
    void updateProgressBar(float *percent);
private slots:
    void aboutToQuitSlot();
    void chartContextMenuRequest(QPoint pos);
    void chartMouseDoubleClickHandler(QMouseEvent *event);
    void chartMousePressHandler(QMouseEvent *event);
    void chartBeforeReplotSlot();
    void chartSelectionChanged();
    void comboBoxSendReturnPressedSlot();
    void hideAllGraphs();
    void hideSelectedGraph();
    void highlighLog(QString searchString);
    void on_action3D_orientation_triggered();
    void on_action50_50_view_triggered();
    void on_actionAbout_Qt_triggered();
    void on_actionFull_chart_triggered();
    void on_actionFull_text_view_triggered();
    void on_actionPlotter_triggered();
    void on_actionPrint_Graph_triggered();
    void on_actionPrint_log_triggered();
    void on_actionQuit_triggered();
    void on_actionSave_As_triggered();
    void on_actionSave_graph_as_triggered();
    void on_actionWhat_s_new_triggered();
    void on_checkBoxAutoLogging_toggled(bool checked);
    void on_checkBoxAutoRefresh_toggled(bool checked);
    void on_checkBoxAutoTrack_toggled(bool checked);
    void on_checkBoxEnableRAMBuffer_toggled(bool checked);
    void on_checkBoxEnableTracer_toggled(bool checked);
    void on_checkBoxExternalTimeReference_toggled(bool checked);
    void on_checkBoxShowLegend_toggled(bool checked);
    void on_checkBoxSyncSystemClock_toggled(bool checked);
    void on_checkBoxWrapText_toggled(bool checked);
    void on_comboBoxGraphDisplayMode_currentIndexChanged(const QString &arg1);
    void on_comboBoxSerialReadMode_currentIndexChanged(int index);
    void on_comboBoxTracerStyle_currentIndexChanged(const QString &arg1);
    void on_comboBoxUDPReceiveMode_currentIndexChanged(const QString &arg1);
    void on_comboBoxUDPSendMode_currentIndexChanged(const QString &arg1);
    void on_lineEditCustomParsingRules_editingFinished();
    void on_lineEditExternalClockLabel_editingFinished();
    void on_lineEditHighlight_returnPressed();
    void on_lineEditLoadFilePath_customContextMenuRequested(const QPoint &pos);
    void on_pushButtonAddDateTime_clicked();
    void on_pushButtonClear_clicked();
    void on_pushButtonClearAll_clicked();
    void on_pushButtonClearGraphs_clicked();
    void on_pushButtonClearHistory_clicked();
    void on_pushButtonClearTable_clicked();
    void on_pushButtonEnablePlot_toggled(bool checked);
    void on_pushButtonFitToContents_clicked();
    void on_pushButtonLoadFile_clicked();
    void on_pushButtonLoadPath_clicked();
    void on_pushButtonLoadRAMBuffer_clicked();
    void on_pushButtonLogging_toggled(bool checked);
    void on_pushButtonLogPath_clicked();
    void on_pushButtonRAMClear_clicked();
    void on_pushButtonRefresh_clicked();
    void on_pushButtonSaveRAMBuffer_clicked();
    void on_pushButtonScrollToButtom_clicked();
    void on_pushButtonSend_clicked();
    void on_pushButtonSerialConnect_toggled(bool checked);
    void on_pushButtonSetDefaultLogPath_clicked();
    void on_pushButtonSetSelectedToGraph_clicked();
    void on_pushButtonTextLogToggle_toggled(bool checked);
    void on_pushButtonUDPConnect_toggled(bool checked);
    void on_spinBoxMaxGraphs_valueChanged(int arg1);
    void on_spinBoxMaxLines_valueChanged(int arg1);
    void on_spinBoxProcessingDelay_valueChanged(int arg1);
    void on_splitterGraphTable_splitterMoved(int pos, int index);
    void on_tableWidgetParsedData_customContextMenuRequested(const QPoint &pos);
    void on_toolButtonAdvancedGraphMenu_clicked();
    void printChangeLog();
    void printLog(QPrinter *printer);
    void printPlot(QPrinter *printer);
    void processSerial();
    void processUDP();
    void showAllGraphs();
    void showPointValue(QMouseEvent *event);
    void showSelectedGraph();
    void showSelectedGraphExclusively();
    void updateDeviceList();
    void on_actionUser_guide_triggered();

private:
    // QCompleter *completer;
    FileReader fileReader;
    Logger fileLogger;
    NetworkUDP networkUDP;
    Parser parser;
    QCPItemTracer *phaseTracer;
    QList<int> missingCount;
    QStringList lineEditCustomParsingRulesAutoCompleteWordList;
    QTimer *radioButtonTimer;
    QTimer *serialDeviceCheckTimer;
    QTimer *serialStringProcessingTimer;
    QTimer *udpStringProcessingTimer;
    Serial serial;
    Ui::MainWindow *ui;
    void addLog(QString text);
    void chartPrintPreview();
    void clearGraphData(bool replot);
    void clearGraphs(bool replot);
    void clearGraphSelection();
    void create3DView();
    void createChart();
    void createChartTracer();
    void createTimers();
    void getFileTimeRange(QFile *file);
    void loadFromRAM(bool loadText);
    void menageAutoCompleteList(QString input);
    void processChart(QStringList labelList, QList<double> numericDataList, QList<long> timeStampsList);
    void processTable(QStringList labels, QList<double> values);
    void saveToRAM(QStringList newlabelList, QList<double> newDataList, QList<long> newTimeList, bool saveText = false, QString text = "");
    void sendDatagram(QString message);
    void sendSerial(QString message);
    void settingsLoadAll();
    void settingsSaveAll();
    void setupGUI();
    void setupTable();
    void writeLogToFile(QString rawLine, QStringList labelList, QList<double> dataList, QList<long> timeList);

protected:
    void keyPressEvent(QKeyEvent *event);
};

enum TAB_INDEX
{
    SERIAL_USB = 0,
    WIFI_UDP,
};

#endif // MAINWINDOW_H