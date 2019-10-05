#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "qcustomplot.h"
#include "config.h"
#include "logger.h"

#include <QMainWindow>
#include <QDateTime>
#include <QtDebug>
#include <QList>
#include <QTimer>
#include <QScrollBar>
#include <QKeyEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QMetaType>
#include <QTextStream>
#include <QElapsedTimer>
#include <QShortcut>
#include <QTime>
#include <QRandomGenerator>
#include <QSettings>
#include <QPaintDevice>
#include <QTextEdit>
#include <QCompleter>
#include <serial.h>
#include <networkudp.h>
#include <parser.h>
#include "filereader.h"
#include <QTime>

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
    void saveToRAM(QStringList newlabelList, QList<double> newDataList, QList<long> newTimeList, bool saveText = false, QString text = "");
public slots:
    void setSelectedLabels(QList<QString> *labels, bool customRules = true);
    void processLoadedFileLine(QString *line, int *progressPercent);
    void processLoadedFile(QString *text);
    void updateProgressBar(float *percent);
private slots:
    void on_pushButtonRefresh_clicked();
    void on_pushButtonSend_clicked();
    void updateDeviceList();
    void on_checkBoxAutoRefresh_toggled(bool checked);
    void processSerial();
    void on_pushButtonScrollToButtom_clicked();
    void on_pushButtonClear_clicked();
    void highlighLog(QString searchString);
    void on_lineEditHighlight_returnPressed();
    void chartMousePressHandler(QMouseEvent *event);
    void showPointValue(QMouseEvent* event);
    void on_checkBoxWrapText_toggled(bool checked);
    void on_checkBoxEnableTracer_toggled(bool checked);
    void chartSelectionChanged();
    void chartMouseDoubleClickHandler(QMouseEvent *event);
    void chartContextMenuRequest(QPoint pos);
    void showAllGraphs();
    void hideAllGraphs();
    void chartRunAutoTrackSlot();
    void on_pushButtonChartClear_clicked();
    void on_pushButtonEnablePlot_toggled(bool checked);
    void comboBoxSendReturnPressedSlot();
    void on_pushButtonClearHistory_clicked();
    void on_comboBoxGraphDisplayMode_currentIndexChanged(const QString &arg1);
    void on_pushButtonClearGraphs_clicked();
    void on_lineEditCustomParsingRules_editingFinished();
    void on_spinBoxMaxGraphs_valueChanged(int arg1);
    void on_spinBoxMaxLines_valueChanged(int arg1);
    void on_checkBoxAutoTrack_toggled(bool checked);
    void on_spinBoxProcessingDelay_valueChanged(int arg1);
    void aboutToQuitSlot();
    void printChangeLog();
    void on_comboBoxTracerStyle_currentIndexChanged(const QString &arg1);
    void hideSelectedGraph();
    void showSelectedGraph();
    void on_pushButtonConnect_toggled(bool checked);
    void showSelectedGraphExclusively();
    void on_actionPlotter_triggered();
    void on_action3D_orientation_triggered();
    void on_actionSave_As_triggered();
    void on_actionFull_chart_triggered();
    void on_actionFull_text_view_triggered();
    void on_action50_50_view_triggered();
    void on_actionWhat_s_new_triggered();
    void on_actionQuit_triggered();
    void printPlot(QPrinter *printer);
    void printLog(QPrinter *printer);
    void on_checkBoxShowLegend_toggled(bool checked);
    void on_actionSave_graph_as_triggered();
    void on_actionPrint_Graph_triggered();
    void on_actionPrint_log_triggered();
    void on_pushButtonUDPConnect_toggled(bool checked);
    void on_comboBoxUDPSendMode_currentIndexChanged(const QString &arg1);
    void on_comboBoxUDPReceiveMode_currentIndexChanged(const QString &arg1);
    void processUDP();
    void on_toolButtonAdvancedGraphMenu_clicked();
    void on_pushButtonLogging_toggled(bool checked);
    void on_checkBoxAutoLogging_toggled(bool checked);
    void on_pushButtonSetDefaultLogPath_clicked();
    void on_pushButtonAddDateTime_clicked();
    void on_pushButtonLogPath_clicked();
    void on_comboBoxSerialReadMode_currentIndexChanged(int index);
    void on_actionAbout_Qt_triggered();
    void on_pushButtonClearTable_clicked();
    void on_tableWidgetParsedData_customContextMenuRequested(const QPoint &pos);
    void on_pushButtonSetSelectedToGraph_clicked();
    void on_splitterGraphTable_splitterMoved(int pos, int index);    
    void on_pushButtonTextLogToggle_toggled(bool checked);
    void on_checkBoxExternalTimeReference_toggled(bool checked);
    void on_lineEditExternalClockLabel_editingFinished();
    void on_pushButtonLoadPath_clicked();
    void on_pushButtonFitToContents_clicked();
    void on_pushButtonLoadRAMBuffer_clicked();
    void on_pushButtonRAMClear_clicked();
    void on_checkBoxEnableRAMBuffer_toggled(bool checked);
    void on_checkBoxSyncSystemClock_toggled(bool checked);
    void on_pushButtonLoadFile_clicked();
    void on_lineEditLoadFilePath_customContextMenuRequested(const QPoint &pos);
    void on_pushButtonSaveRAMBuffer_clicked();    
private:
    Ui::MainWindow *ui;
    QTimer *serialDeviceCheckTimer;
    QTimer *radioButtonTimer;
    QTimer *serialStringProcessingTimer;
    QTimer *udpStringProcessingTimer;
    QCPItemTracer *phaseTracer;
    QStringList lineEditCustomParsingRulesAutoCompleteWordList;
    QList<int> missingCount;
    // QCompleter *completer;
    Logger fileLogger;
    Serial serial;
    NetworkUDP networkUDP;
    Parser parser;
    FileReader fileReader;
    void addLog(QString text);
    void createTimers();
    void createChart();
    void createChartTracer();
    void clearGraphs(bool replot);
    void setupGUI();
    void settingsLoadAll();
    void settingsSaveAll();
    void menageAutoCompleteList(QString input);
    void clearGraphSelection();
    void create3DView();
    void clearGraphData(bool replot);
    void chartPrintPreview();
    void sendDatagram(QString message);
    void processTable(QStringList labels, QList<double> values);
    void sendSerial(QString message);
    void setupTable();
    void processChart(QStringList labelList, QList<double> numericDataList, QList<long> timeStampsList);
    void loadFromRAM(bool loadText);
    void writeLogToFile(QString rawLine, QStringList labelList, QList<double> dataList, QList<long> timeList);
    void getFileTimeRange(QFile *file);
protected:
    void keyPressEvent(QKeyEvent *event);
};

enum TAB_INDEX{
    SERIAL_USB = 0,
    WIFI_UDP,
};

#endif // MAINWINDOW_H
