#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    createTimers();
    createChart();
    setupTable();
    setupGUI();

    settingsLoadAll();

    infoDialog.setFixedSize(800, 600);

    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(on_aboutToQuitSlot()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // if (ui->pushButtonSerialConnect->isChecked() || ui->pushButtonUDPConnect->isChecked())

    if (serial.isOpen() || networkUDP.isOpen())
    {
        QMessageBox::StandardButton resBtn = QMessageBox::question(this, "About to exit...", tr("Connection open. Are you sure ? \n"),
                                                                   QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (resBtn == QMessageBox::No)
            event->ignore();
        else
            event->accept();
    }
}

void MainWindow::on_aboutToQuitSlot()
{
    qDebug() << "on_aboutToQuitSlot";
    settingsSaveAll();
}

void MainWindow::createTimers()
{
    this->serialDeviceCheckTimer = new QTimer(this);
    this->radioButtonTimer = new QTimer(this);
    this->serialStringProcessingTimer = new QTimer(this);
    this->udpStringProcessingTimer = new QTimer(this);

    connect(serialDeviceCheckTimer, SIGNAL(timeout()), this, SLOT(on_updateSerialDeviceList()));
    connect(radioButtonTimer, &QTimer::timeout, this, [=]() { ui->radioButtonDeviceUpdate->setChecked(false); });
}

void MainWindow::setupGUI()
{
    // ----------------------- MainWindow ----------------------- //
    {
        connect(ui->comboBoxSend->lineEdit(), SIGNAL(returnPressed()), this, SLOT(on_comboBoxSendReturnPressedSlot()));
        this->setWindowTitle(this->windowTitle() + " " + VERSION);
    }
    // ---------------------------------------------------------- //

    // ----------------------- textBrowserLogs ----------------------- //
    {
        ui->textBrowserLogs->document()->setMaximumBlockCount(ui->spinBoxMaxLines->value());

        if (ui->checkBoxWrapText->isChecked() == true)
            ui->textBrowserLogs->setLineWrapMode(QPlainTextEdit::LineWrapMode::WidgetWidth);
        else
            ui->textBrowserLogs->setLineWrapMode(QPlainTextEdit::LineWrapMode::NoWrap);

        //   highlighter = new Highlighter(ui->textBrowserLogs->document());
    }
    // ---------------------------------------------------------------- //

    // ----------------------- standardBaudRates ----------------------- //
    {
        foreach (auto item, serial.getAvailibleBaudRates())
            ui->comboBoxBaudRates->addItem(QString::number(item));

        ui->comboBoxBaudRates->setCurrentIndex(ui->comboBoxBaudRates->count() / 2); // select middle
    }
    // ----------------------------------------------------------------- //

    if (ui->checkBoxAutoRefresh->isChecked())
    {
        serialDeviceCheckTimer->start(SERIAL_DEVICE_CHECK_TIMER_INTERVAL);
        ui->pushButtonRefresh->setEnabled(false);
    }
    else
    {
        ui->pushButtonRefresh->setEnabled(true);
    }

    // ----------------------- comboBoxGraphDisplayMode ----------------------- //
    {
        if (ui->comboBoxGraphDisplayMode->currentIndex() == 0)
            ui->lineEditCustomParsingRules->setEnabled(false);
        else
            ui->lineEditCustomParsingRules->setEnabled(true);
    }
    // ----------------------------------------------------------------- //

    ui->comboBoxExternalTimeFormat->addItem("[ms]");
    ui->comboBoxExternalTimeFormat->setCurrentIndex(0);
    ui->lineEditLoadFilePath->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    ui->lineEditSaveFileName->setText("Log.txt");
    ui->lineEditSaveLogPath->setText(qApp->applicationDirPath() + "/Logs");
    ui->splitterGraphTable->setSizes({this->height(), 0});

    emit on_checkBoxAutoLogging_toggled(ui->checkBoxAutoLogging->isChecked());
    emit on_checkBoxShowLegend_toggled(ui->checkBoxShowLegend->isChecked());
    emit on_comboBoxClockSource_currentIndexChanged(ui->comboBoxClockSource->currentIndex());

    on_updateSerialDeviceList();
}

void MainWindow::createChart()
{
    // ui->widgetChart->setOpenGl(false);
    ui->splitterReceivedData->setSizes(QList<int>({200, 400})); // default
    ui->widgetChart->setInteractions(QCP::iRangeZoom |
                                     QCP::iRangeDrag |
                                     QCP::iSelectPlottables |
                                     QCP::iSelectLegend);

    QSharedPointer<QCPAxisTickerTime> xTicker(new QCPAxisTickerTime);
    QSharedPointer<QCPAxisTicker> yTicker(new QCPAxisTicker);
    xTicker->setTimeFormat("%h:%m:%s:%z");
    xTicker->setTickCount(5);
    yTicker->setTickCount(10);
    yTicker->setTickStepStrategy(QCPAxisTicker::TickStepStrategy::tssReadability);
    ui->widgetChart->xAxis->setTicker(xTicker);
    ui->widgetChart->yAxis->setTicker(yTicker);
    ui->widgetChart->axisRect()->setupFullAxesBox(true);
    ui->widgetChart->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignBottom | Qt::AlignLeft);
    ui->widgetChart->legend->setVisible(true);
    ui->widgetChart->legend->setFont(QFont("MS Shell Dlg 2", 8, QFont::Thin, false));
    ui->widgetChart->legend->setIconSize(15, 10);
    ui->widgetChart->legend->setSelectableParts(QCPLegend::SelectablePart::spItems); // legend box shall not be selectable, only legend items
    ui->widgetChart->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    ui->widgetChart->xAxis->setRange(-1.0, ui->spinBoxScrollingTimeRange->value());
    ui->widgetChart->yAxis->setRange(-1.0, 1.0);

    connect(ui->widgetChart, SIGNAL(mouseDoubleClick(QMouseEvent *)), this, SLOT(on_chartMouseDoubleClickHandler(QMouseEvent *)));
    connect(ui->widgetChart, SIGNAL(mousePress(QMouseEvent *)), this, SLOT(on_chartMousePressHandler(QMouseEvent *)));
    connect(ui->widgetChart, SIGNAL(mouseMove(QMouseEvent *)), this, SLOT(on_chartMouseMoveHandler(QMouseEvent *)));

    connect(ui->widgetChart, SIGNAL(selectionChangedByUser()), this, SLOT(on_chartSelectionChanged()));
    connect(ui->widgetChart, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(on_chartContextMenuRequest(QPoint)));
    connect(ui->widgetChart, SIGNAL(beforeReplot()), this, SLOT(on_chartBeforeReplotSlot()));

    //  ui->widgetChart->setNotAntialiasedElements(QCP::aeAll);

    if (ui->checkBoxEnableTracer->isChecked())
    {
        createChartTracer();
        connect(ui->widgetChart, SIGNAL(mouseMove(QMouseEvent *)), this, SLOT(on_tracerShowPointValue(QMouseEvent *)));
    }

    ui->textBrowserLogs->setHighlightEnabled(false);
    emit on_comboBoxLogFormat_currentIndexChanged(ui->comboBoxLogFormat->currentIndex());
}

void MainWindow::setupTable()
{
    ui->tableWidgetParsedData->clear();
    ui->tableWidgetParsedData->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    ui->tableWidgetParsedData->setColumnCount(2);
    ui->tableWidgetParsedData->setHorizontalHeaderItem(0, new QTableWidgetItem("Name"));
    ui->tableWidgetParsedData->setHorizontalHeaderItem(1, new QTableWidgetItem("Current Value"));
    //    ui->tableWidgetParsedData->setHorizontalHeaderItem(2, new QTableWidgetItem("Max"));
    //    ui->tableWidgetParsedData->setHorizontalHeaderItem(3, new QTableWidgetItem("Min"));

    ui->tableWidgetLogTable->clear();
    ui->tableWidgetLogTable->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    ui->tableWidgetLogTable->setColumnCount(1);
    ui->tableWidgetLogTable->setHorizontalHeaderItem(0, new QTableWidgetItem("Time:"));
    ui->tableWidgetLogTable->setSortingEnabled(true);
}

void MainWindow::settingsLoadAll()
{
    QSettings appSettings(QSettings::Format::IniFormat,
                          QSettings::Scope::UserScope,
                          "QtSerialMonitor", "QtSerialMonitor"); // Settings

    // ----- Info ----- //
    {
        if (appSettings.value("Info/version").value<QString>() != VERSION)
        {
            on_printIntroChangelog();
        }

        if (appSettings.value("Info/organizationName").value<QString>() != appSettings.organizationName() ||
                appSettings.value("Info/applicationName").value<QString>() != appSettings.applicationName())
        {
            qDebug() << "Abort loading settings ! organizationName or applicationName incorrect. Config file might be missing.";
            addLog("App >>\t Error loading settings. Config file incorrect !", true);
            return;
        }
    }
    // ------------------------- //

    // ----- Window size ----- //
    {
        //        QStringList windowSize;
        //        windowSize = appSettings.value("layout/windowSize.size").value<QString>().split(QRegExp("\\s+"), QString::SplitBehavior::SkipEmptyParts);

        //        if (!windowSize.isEmpty())
        //            this->resize(windowSize.first().trimmed().toInt(), windowSize.last().trimmed().toInt());
    }
    // ------------------------- //

    // ----- splitterSizes ----- //
    {
        QStringList splitterSizes;

        splitterSizes = appSettings.value("layout/splitterReceivedData.sizes").value<QString>().split(QRegExp("\\s+"), QString::SplitBehavior::SkipEmptyParts);
        if (!splitterSizes.isEmpty())
            ui->splitterReceivedData->setSizes(QList<int>({splitterSizes.first().trimmed().toInt(), splitterSizes.last().trimmed().toInt()}));

        splitterSizes = appSettings.value("layout/splitterGraphTable.sizes").value<QString>().split(QRegExp("\\s+"), QString::SplitBehavior::SkipEmptyParts);
        if (!splitterSizes.isEmpty())
            ui->splitterGraphTable->setSizes(QList<int>({splitterSizes.first().trimmed().toInt(), splitterSizes.last().trimmed().toInt()}));

        if (ui->splitterGraphTable->sizes().last() > 0)
            ui->toolButtonAdvancedGraphMenu->setArrowType(Qt::ArrowType::DownArrow);
        else
            ui->toolButtonAdvancedGraphMenu->setArrowType(Qt::ArrowType::UpArrow);
    }
    // ------------------------- //

    // ----- comboBoxSendItems ----- //
    {
        ui->comboBoxSend->addItems(QStringList(appSettings.value("data/comboBoxSendHistory").value<QString>().split(QRegExp("\\n+"), QString::SplitBehavior::SkipEmptyParts)));
        ui->comboBoxSend->lineEdit()->clear();
    }
    // ------------------------- //

    // ----- Other GUI Elements ----- //
    {
        ui->checkBoxAppendDate->setChecked(appSettings.value("GUI_Elements/checkBoxAppendDate.isChecked").value<bool>());
        ui->checkBoxAutoLogging->setChecked(appSettings.value("GUI_Elements/checkBoxAutoLogging.isChecked").value<bool>());
        ui->checkBoxAutoRefresh->setChecked(appSettings.value("GUI_Elements/checkBoxAutoRefresh.isChecked").value<bool>());
        ui->checkBoxAutoRescaleY->setChecked(appSettings.value("GUI_Elements/checkBoxAutoRescaleY.isChecked").value<bool>());
        ui->checkBoxAutoSaveBuffer->setChecked(appSettings.value("GUI_Elements/checkBoxAutoSaveBuffer.isChecked", true).value<bool>());
        ui->checkBoxAutoTrack->setChecked(appSettings.value("GUI_Elements/checkBoxAutoTrack.isChecked").value<bool>());
        ui->checkBoxDTR->setChecked(appSettings.value("GUI_Elements/checkBoxDTR.isChecked").value<bool>());
        ui->checkBoxEnableTracer->setChecked(appSettings.value("GUI_Elements/checkBoxEnableTracer.isChecked").value<bool>());
        ui->checkBoxRAMClearChart->setChecked(appSettings.value("GUI_Elements/checkBoxRAMClearChart.isChecked").value<bool>());
        ui->checkBoxScrollToButtom->setChecked(appSettings.value("GUI_Elements/checkBoxScrollToButtom.isChecked").value<bool>());
        ui->checkBoxSendKey->setChecked(appSettings.value("GUI_Elements/checkBoxSendKey.isChecked").value<bool>());
        ui->checkBoxShowControlChars->setChecked(appSettings.value("GUI_Elements/checkBoxShowControlChars.isChecked", false).value<bool>());
        ui->checkBoxSimplifyLog->setChecked(appSettings.value("GUI_Elements/checkBoxSimplifyLog.isChecked").value<bool>());
        ui->checkBoxTableAutoResize->setChecked(appSettings.value("GUI_Elements/checkBoxTableAutoResize.isChecked", true).value<bool>());
        ui->checkBoxTruncateFileOnSave->setChecked(appSettings.value("GUI_Elements/checkBoxTruncateFileOnSave.isChecked", false).value<bool>());
        ui->checkBoxWrapText->setChecked(appSettings.value("GUI_Elements/checkBoxWrapText.isChecked").value<bool>());
        ui->checkBoxAutoScrollLogTable->setChecked(appSettings.value("GUI_Elements/checkBoxAutoScrollLogTable.isChecked", true).value<bool>());
        ui->checkBoxAutoSizeColumnsLogTable->setChecked(appSettings.value("GUI_Elements/checkBoxAutoSizeColumnsLogTable.isChecked", true).value<bool>());
        ui->checkBoxScrollLogEnableSorting->setChecked(appSettings.value("GUI_Elements/checkBoxScrollLogEnableSorting.isChecked", true).value<bool>());        ui->checkBoxScrollLogEnableSorting->setChecked(appSettings.value("GUI_Elements/checkBoxScrollLogEnableSorting.isChecked", true).value<bool>());
        ui->comboBoxAddTextMode->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxAddTextMode.currentIndex").value<int>());
        ui->comboBoxBaudRates->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxBaudRates.currentIndex").value<int>());
        ui->comboBoxClockSource->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxClockSource.currentIndex").value<int>());
        ui->comboBoxDataBits->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxDataBits.currentIndex").value<int>());
        ui->comboBoxExternalTimeFormat->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxExternalTimeFormat.currentIndex").value<int>());
        ui->comboBoxFlowControl->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxFlowControl.currentIndex").value<int>());
        ui->comboBoxGraphDisplayMode->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxGraphDisplayMode.currentIndex").value<int>());
        ui->comboBoxLoggingMode->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxLoggingMode.currentIndex").value<int>());
        ui->comboBoxMessagesDisplayMode->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxMessagesDisplayMode.currentIndex").value<int>());
        ui->comboBoxParity->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxParity.currentIndex").value<int>());
        ui->comboBoxRAMLoadMode->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxRAMLoadMode.currentIndex").value<int>());
        ui->comboBoxRAMSaveMode->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxRAMSaveMode.currentIndex").value<int>());
        ui->comboBoxSerialReadMode->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxSerialReadMode.currentIndex").value<int>());
        ui->comboBoxStopBits->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxStopBits.currentIndex").value<int>());
        ui->comboBoxTextProcessing->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxTextProcessing.currentIndex").value<int>());
        ui->comboBoxTracerStyle->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxTracerStyle.currentIndex").value<int>());
        ui->comboBoxUDPReceiveMode->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxUDPReceiveMode.currentIndex").value<int>());
        ui->comboBoxUDPSendMode->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxUDPSendMode.currentIndex").value<int>());
        ui->tabWidgetTableView->setCurrentIndex(appSettings.value("GUI_Elements/tabWidgetTableView.currentIndex").value<int>());
        ui->lineEditCustomParsingRules->setText(appSettings.value("data/lineEditCustomParsingRules.text").value<QString>());
        ui->lineEditExternalClockLabel->setText(appSettings.value("data/lineEditExternalClockLabel.text").value<QString>());
        ui->lineEditLoadFilePath->setText(appSettings.value("data/lineEditLoadFilePath.text").value<QString>());
        ui->lineEditSaveFileName->setText(appSettings.value("data/lineEditSaveFileName.text").value<QString>());
        ui->lineEditSaveLogPath->setText(appSettings.value("data/lineEditSaveLogPath.text").value<QString>());
        ui->lineEditUDPTargetIP->setText(appSettings.value("data/lineEditUDPTargetIP.text").value<QString>());
        ui->pushButtonEnablePlot->setChecked(appSettings.value("GUI_Elements/pushButtonEnablePlot.isChecked", false).value<bool>());
        ui->pushButtonEnableTableLog->setChecked(appSettings.value("GUI_Elements/pushButtonEnableTableLog.isChecked", true).value<bool>());
        ui->spinBoxMaxGraphs->setValue(appSettings.value("layout/spinBoxMaxGraphs.value").value<int>());
        ui->spinBoxMaxLines->setValue(appSettings.value("layout/spinBoxMaxLines.value").value<int>());
        ui->spinBoxMaxTimeRange->setValue(appSettings.value("layout/spinBoxMaxTimeRange.value").value<int>());
        ui->spinBoxProcessingDelay->setValue(appSettings.value("layout/spinBoxProcessingDelay.value").value<int>());
        ui->spinBoxRemoveOldLabels->setValue(appSettings.value("layout/spinBoxRemoveOldLabels.value").value<int>());
        ui->spinBoxScrollingTimeRange->setValue(appSettings.value("layout/spinBoxScrollingTimeRange.value").value<int>());
        ui->spinBoxUDPReceivePort->setValue(appSettings.value("layout/spinBoxUDPReceivePort.value").value<int>());
        ui->spinBoxUDPTargetPort->setValue(appSettings.value("layout/spinBoxUDPTargetPort.value").value<int>());
        ui->spinBoxMaxRowsLogTable->setValue(appSettings.value("layout/spinBoxMaxRowsLogTable.value").value<int>());
        ui->tabWidgetControlSection->setCurrentIndex(appSettings.value("GUI_Elements/tabWidgetControlSection.currentIndex").value<int>());

        if (ui->lineEditLoadFilePath->text().isEmpty() == false)
        {
            QFile file(ui->lineEditLoadFilePath->text());
            QList<QTime> timeRange = fileReader.getFileTimeRange(&file);
            ui->timeEditMinParsingTime->setTime(timeRange.first());
            ui->timeEditMaxParsingTime->setTime(timeRange.last());
        }

        qDebug() << "Loaded settings";
    }
}

void MainWindow::settingsSaveAll()
{
    QSettings appSettings(QSettings::Format::IniFormat,
                          QSettings::Scope::UserScope,
                          "QtSerialMonitor", "QtSerialMonitor"); // Settings

    // ----- Info ----- //
    {
        appSettings.setValue("Info/version", VERSION);
        appSettings.setValue("Info/organizationName", appSettings.organizationName());
        appSettings.setValue("Info/applicationName", appSettings.applicationName());
    }
    // ------------------------- //

    // ----- Window size ----- //
    {
        //        QString windowSize;
        //        windowSize = QString::number(this->size().width()) + " " + QString::number(this->size().height());
        //        appSettings.setValue("layout/windowSize.size", windowSize);
    }
    // ------------------------- //

    // ----- splitterSizes ----- //
    {
        QString splitterSizes;

        splitterSizes = QString::number(ui->splitterReceivedData->sizes().first()) + " " + QString::number(ui->splitterReceivedData->sizes().last());
        appSettings.setValue("layout/splitterReceivedData.sizes", splitterSizes);

        splitterSizes = QString::number(ui->splitterGraphTable->sizes().first()) + " " + QString::number(ui->splitterGraphTable->sizes().last());
        appSettings.setValue("layout/splitterGraphTable.sizes", splitterSizes);
    }
    // ------------------------- //

    // ----- GUI Elements ----- //
    {
        appSettings.setValue("data/lineEditCustomParsingRules.text", ui->lineEditCustomParsingRules->text());
        appSettings.setValue("data/lineEditExternalClockLabel.text", ui->lineEditExternalClockLabel->text());
        appSettings.setValue("data/lineEditLoadFilePath.text", ui->lineEditLoadFilePath->text());
        appSettings.setValue("data/lineEditSaveFileName.text", ui->lineEditSaveFileName->text());
        appSettings.setValue("data/lineEditSaveLogPath.text", ui->lineEditSaveLogPath->text());
        appSettings.setValue("data/lineEditUDPTargetIP.text", ui->lineEditUDPTargetIP->text());
        appSettings.setValue("GUI_Elements/checkBoxAppendDate.isChecked", ui->checkBoxAppendDate->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxAutoLogging.isChecked", ui->checkBoxAutoLogging->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxAutoRefresh.isChecked", ui->checkBoxAutoRefresh->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxAutoRescaleY.isChecked", ui->checkBoxAutoRescaleY->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxAutoSaveBuffer.isChecked", ui->checkBoxAutoSaveBuffer->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxAutoTrack.isChecked", ui->checkBoxAutoTrack->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxDTR.isChecked", ui->checkBoxDTR->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxEnableTracer.isChecked", ui->checkBoxEnableTracer->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxRAMClearChart.isChecked", ui->checkBoxRAMClearChart->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxScrollToButtom.isChecked", ui->checkBoxScrollToButtom->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxSendKey.isChecked", ui->checkBoxSendKey->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxShowControlChars.isChecked", ui->checkBoxShowControlChars->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxSimplifyLog.isChecked", ui->checkBoxSimplifyLog->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxTableAutoResize.isChecked", ui->checkBoxTableAutoResize->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxTruncateFileOnSave.isChecked", ui->checkBoxTruncateFileOnSave->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxWrapText.isChecked", ui->checkBoxWrapText->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxScrollLogEnableSorting.isChecked", ui->checkBoxScrollLogEnableSorting->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxAutoSizeColumnsLogTable.isChecked", ui->checkBoxAutoSizeColumnsLogTable->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxAutoScrollLogTable.isChecked", ui->checkBoxAutoScrollLogTable->isChecked());
        appSettings.setValue("GUI_Elements/comboBoxAddTextMode.currentIndex", ui->comboBoxAddTextMode->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxBaudRates.currentIndex", ui->comboBoxBaudRates->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxClockSource.currentIndex", ui->comboBoxClockSource->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxDataBits.currentIndex", ui->comboBoxDataBits->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxExternalTimeFormat.currentIndex", ui->comboBoxExternalTimeFormat->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxFlowControl.currentIndex", ui->comboBoxFlowControl->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxGraphDisplayMode.currentIndex", ui->comboBoxGraphDisplayMode->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxLoggingMode.currentIndex", ui->comboBoxLoggingMode->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxMessagesDisplayMode.currentIndex", ui->comboBoxMessagesDisplayMode->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxParity.currentIndex", ui->comboBoxParity->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxRAMLoadMode.currentIndex", ui->comboBoxRAMLoadMode->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxRAMSaveMode.currentIndex", ui->comboBoxRAMSaveMode->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxSerialReadMode.currentIndex", ui->comboBoxSerialReadMode->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxStopBits.currentIndex", ui->comboBoxStopBits->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxTextProcessing.currentIndex", ui->comboBoxTextProcessing->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxTracerStyle.currentIndex", ui->comboBoxTracerStyle->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxUDPReceiveMode.currentIndex", ui->comboBoxUDPReceiveMode->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxUDPSendMode.currentIndex", ui->comboBoxUDPSendMode->currentIndex());
        appSettings.setValue("GUI_Elements/tabWidgetTableView.currentIndex", ui->tabWidgetTableView->currentIndex());
        appSettings.setValue("GUI_Elements/pushButtonEnablePlot.isChecked", ui->pushButtonEnablePlot->isChecked());
        appSettings.setValue("GUI_Elements/tabWidgetControlSection.currentIndex", ui->tabWidgetControlSection->currentIndex());
        appSettings.setValue("layout/spinBoxMaxGraphs.value", ui->spinBoxMaxGraphs->value());
        appSettings.setValue("layout/spinBoxMaxLines.value", ui->spinBoxMaxLines->value());
        appSettings.setValue("layout/spinBoxMaxTimeRange.value", ui->spinBoxMaxTimeRange->value());
        appSettings.setValue("layout/spinBoxProcessingDelay.value", ui->spinBoxProcessingDelay->value());
        appSettings.setValue("layout/spinBoxRemoveOldLabels.value", ui->spinBoxRemoveOldLabels->value());
        appSettings.setValue("layout/spinBoxScrollingTimeRange.value", ui->spinBoxScrollingTimeRange->value());
        appSettings.setValue("layout/spinBoxUDPReceivePort.value", ui->spinBoxUDPReceivePort->value());
        appSettings.setValue("layout/spinBoxUDPTargetPort.value", ui->spinBoxUDPTargetPort->value());
        appSettings.setValue("layout/spinBoxMaxRowsLogTable.value", ui->spinBoxMaxRowsLogTable->value());
        appSettings.setValue("layout/pushButtonEnableTableLog.isChecked", ui->pushButtonEnableTableLog->isChecked());
    }
    // ------------------------- //

    // ----- comboBoxSendItems ----- //
    {
        QString comboBoxSendItems;
        for (auto i = 0; i < ui->comboBoxSend->count(); ++i)
            comboBoxSendItems.append(ui->comboBoxSend->itemText(i) + "\n");
        qDebug() << comboBoxSendItems;
        appSettings.setValue("data/comboBoxSendHistory", comboBoxSendItems);
    }
    // ------------------------- //

    appSettings.sync();

    qDebug() << "Save all settings";
}

void MainWindow::processTable(QStringList labels, QList<double> values)
{
    static QList<int> tableMissingCount;

    foreach (auto label, labels)
    {
        if (ui->tableWidgetParsedData->findItems(label, Qt::MatchFlag::MatchExactly).count() < 1)
        {
            ui->tableWidgetParsedData->insertRow(ui->tableWidgetParsedData->rowCount());

            QTableWidgetItem *newLabel = new QTableWidgetItem(label);
            ui->tableWidgetParsedData->setItem(ui->tableWidgetParsedData->rowCount() - 1, 0, newLabel);
            tableMissingCount.append(0);
        }

        for (auto i = 0; i < ui->tableWidgetParsedData->rowCount(); ++i)
        {
            if (ui->tableWidgetParsedData->item(i, 0)->text() == label)
            {
                QTableWidgetItem *newValue = new QTableWidgetItem(QString::number(values[labels.indexOf(label)]));
                ui->tableWidgetParsedData->setItem(i, 1, newValue);
                break;
            }
        }

        if (ui->spinBoxRemoveOldLabels->value() > 0)
        {
            for (auto i = 0; i < ui->tableWidgetParsedData->rowCount(); ++i)
            {
                if (ui->tableWidgetParsedData->item(i, 0)->text() == label)
                    tableMissingCount[i] = 0;
                else
                    tableMissingCount[i]++;

                if (tableMissingCount[i] > ui->spinBoxRemoveOldLabels->value())
                {
                    ui->tableWidgetParsedData->removeRow(i);
                    tableMissingCount.removeAt(i);
                }
            }
        }
    }

    if (ui->checkBoxTableAutoResize->isChecked())
    {
        ui->tableWidgetParsedData->resizeColumnsToContents();
        ui->tableWidgetParsedData->resizeRowsToContents();
    }
}

void MainWindow::processLogTable(QList<long> timeTable, QStringList labelTable, QList<double> valueTable)
{
    if (ui->pushButtonEnableTableLog->isChecked())
        return;

    //    static QList<int> tableMissingCount;
    unsigned long oldRowCount = ui->tableWidgetLogTable->rowCount();

    QStringList firstRow;
    for (auto i = 0; i < ui->tableWidgetLogTable->columnCount(); ++i)
        firstRow.append(ui->tableWidgetLogTable->horizontalHeaderItem(i)->text().trimmed());

    bool resizeFlag = false;

    foreach (auto label, labelTable)
    {
        if (firstRow.contains(label) == false) // optimise
        {
            ui->tableWidgetLogTable->setColumnCount(ui->tableWidgetLogTable->columnCount() + 1);
            ui->tableWidgetLogTable->setHorizontalHeaderItem(ui->tableWidgetLogTable->columnCount() - 1, new QTableWidgetItem(label.trimmed()));

            firstRow.clear();
            for (auto i = 0; i < ui->tableWidgetLogTable->columnCount(); ++i)
                firstRow.append(ui->tableWidgetLogTable->horizontalHeaderItem(i)->text().trimmed());

            resizeFlag = true;
        }
        else
        {
            if (oldRowCount == ui->tableWidgetLogTable->rowCount())
                ui->tableWidgetLogTable->setRowCount(oldRowCount + 1);

            ui->tableWidgetLogTable->setItem(oldRowCount, 0, new QTableWidgetItem(QTime::fromMSecsSinceStartOfDay(timeTable[labelTable.indexOf(label)]).toString(parser.searchTimeFormatList[0])));
            ui->tableWidgetLogTable->setItem(oldRowCount, firstRow.indexOf(label), new QTableWidgetItem(QString::number(valueTable[labelTable.indexOf(label)])));
        }
    }

    if (ui->checkBoxAutoScrollLogTable->isChecked())
        ui->tableWidgetLogTable->scrollToBottom();

    if (ui->checkBoxAutoSizeColumnsLogTable->isChecked() && resizeFlag)
        ui->tableWidgetLogTable->resizeColumnsToContents();

    if (ui->spinBoxMaxRowsLogTable->value() > 0)
    {
        while (ui->tableWidgetLogTable->rowCount() > ui->spinBoxMaxRowsLogTable->value())
        {
            ui->tableWidgetLogTable->removeRow(0);
        }
    }
}

void MainWindow::on_printIntroChangelog() // TODO
{
    ui->pushButtonTextLogToggle->setChecked(false);
    ui->textBrowserLogs->appendPlainText(INTRO_TEXT);
    ui->textBrowserLogs->appendPlainText("\n" CHANGELOG_TEXT);

    //    ui->textBrowserLogs->horizontalScrollBar()->setValue(0);
}

void MainWindow::on_comboBoxSendReturnPressedSlot()
{
    sendMessageLineEdit(ui->tabWidgetControlSection->currentIndex());
}

void MainWindow::sendMessageLineEdit(int mode)
{
    if (mode == 0)
        sendSerial(ui->comboBoxSend->currentText());
    else if (mode == 1)
        sendUDPDatagram(ui->comboBoxSend->currentText());
    else
    {
        sendSerial(ui->comboBoxSend->currentText()); // TODO
        sendUDPDatagram(ui->comboBoxSend->currentText());
    }

    if (ui->comboBoxMessagesDisplayMode->currentIndex() == 0)
        addLog("\n >> " + ui->comboBoxSend->currentText(), ui->comboBoxAddTextMode->currentIndex());

    ui->comboBoxSend->setCurrentText("");
    ui->comboBoxSend->model()->sort(0, Qt::SortOrder::AscendingOrder); // sort alphabetically
}

// Auto-scroll X and rescale Y
void MainWindow::on_chartBeforeReplotSlot()
{
    static bool isChartEmpty = false;
    if (ui->widgetChart->graphCount() > 0 && isChartEmpty == true)
    {
        isChartEmpty = false;
        ui->widgetChart->rescaleAxes(); // center plot if new graphs added to empty chart
    }
    else if (ui->widgetChart->graphCount() == 0)
    {
        isChartEmpty = true;
    }

    if (ui->checkBoxAutoTrack->isChecked() && ui->widgetChart->graphCount() > 0)
    {
        if (parser.getListTimeStamp().count() > 0)
        {
            ui->widgetChart->xAxis->setRange((parser.getListTimeStamp().last() / 1000.0) + (ui->spinBoxScrollingTimeRange->value() * 0.05),
                                             ui->spinBoxScrollingTimeRange->value(), Qt::AlignRight);
        }

        if (ui->checkBoxAutoRescaleY->isChecked())
        {
            bool atLeastOneGraphVisible = false, atLeastOneGraphIsNotFlat = false;

            for (auto i = 0; i < ui->widgetChart->graphCount(); ++i)
            {
                if (ui->widgetChart->graph(i)->visible())
                {
                    atLeastOneGraphVisible = true;

                    bool foundRange = false;
                    QCPRange valueRange = ui->widgetChart->graph(i)->data().data()->valueRange(foundRange);

                    if (foundRange == true && abs(valueRange.upper - valueRange.lower) >= 0.1)
                        atLeastOneGraphIsNotFlat = true;
                }
            }

            if (atLeastOneGraphVisible && atLeastOneGraphIsNotFlat)
            {
                ui->widgetChart->yAxis->rescale(true);
                ui->widgetChart->yAxis->scaleRange(1.20); // leave some space at the top and buttom of the chart.
            }
        }
    }
    else
    {
        // do nothing ?
    }
}

void MainWindow::on_chartContextMenuRequest(QPoint pos) // right click on chart
{
    QMenu *menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    if (ui->widgetChart->legend->selectTest(pos, false) >= 0) // context menu on legend requested
    {
        on_clearGraphSelection();

        for (auto i = 0; i < ui->widgetChart->legend->itemCount(); ++i)
        {
            if (ui->widgetChart->legend->item(i)->selectTest(pos, false) >= 0)
            {
                ui->widgetChart->legend->item(i)->setSelected(true);
                menu->addAction("Show excusively", this, SLOT(on_showSelectedGraphExclusively()));

                if (ui->widgetChart->graph(i)->visible())
                    menu->addAction("Hide graph", this, SLOT(on_hideSelectedGraph()));
                else
                    menu->addAction("Show graph", this, SLOT(on_showSelectedGraph()));
            }
        }
    }
    else
    {
        for (auto i = 0; i < ui->widgetChart->legend->itemCount(); ++i) // context menu requested eslewhere on the graph
        {
            if (ui->widgetChart->legend->item(i)->selected())
            {
                menu->addAction("Show excusively", this, SLOT(on_showSelectedGraphExclusively()));

                if (ui->widgetChart->graph(i)->visible())
                    menu->addAction("Hide graph", this, SLOT(on_hideSelectedGraph()));
                else
                    menu->addAction("Show graph", this, SLOT(on_showSelectedGraph()));
            }
        }
    }

    if (ui->widgetChart->graphCount() > 0)
    {
        menu->addAction("Show all graphs", this, SLOT(on_showAllGraphs()));
        menu->addAction("Hide all graphs", this, SLOT(on_hideAllGraphs()));
    }

    menu->popup(ui->widgetChart->mapToGlobal(pos));
    ui->widgetChart->replot();
}

void MainWindow::on_showSelectedGraphExclusively()
{
    on_hideAllGraphs();

    for (auto i = 0; i < ui->widgetChart->graphCount(); ++i)
    {
        if (ui->widgetChart->graph(i)->selected() || ui->widgetChart->legend->item(i)->selected())
        {
            ui->widgetChart->graph(i)->setVisible(true);
            ui->widgetChart->legend->item(i)->setTextColor(Qt::GlobalColor::black);
            ui->widgetChart->replot();

            return;
        }
    }
}

void MainWindow::on_showAllGraphs()
{
    for (auto i = 0; i < ui->widgetChart->legend->itemCount(); ++i)
    {
        ui->widgetChart->graph(i)->setVisible(true);
        ui->widgetChart->legend->item(i)->setTextColor(Qt::GlobalColor::black);
    }

    ui->widgetChart->replot();
}

void MainWindow::on_clearGraphSelection()
{
    for (auto i = 0; i < ui->widgetChart->graphCount(); ++i)
    {
        ui->widgetChart->legend->item(i)->setSelected(false);
    }
}

void MainWindow::on_hideSelectedGraph()
{
    for (auto i = 0; i < ui->widgetChart->legend->itemCount(); ++i)
    {
        if (ui->widgetChart->legend->item(i)->selected())
        {
            ui->widgetChart->graph(i)->setVisible(false);
            ui->widgetChart->legend->item(i)->setTextColor(Qt::GlobalColor::gray);
        }
    }

    on_clearGraphSelection();
    ui->widgetChart->replot();
}

void MainWindow::on_showSelectedGraph()
{
    for (auto i = 0; i < ui->widgetChart->legend->itemCount(); ++i)
    {
        if (ui->widgetChart->legend->item(i)->selected())
        {
            ui->widgetChart->graph(i)->setVisible(true);
            ui->widgetChart->legend->item(i)->setTextColor(Qt::GlobalColor::black);
        }
    }

    on_clearGraphSelection();
    ui->widgetChart->replot();
}

void MainWindow::on_hideAllGraphs()
{
    if (ui->widgetChart->graphCount() > 0)
    {
        for (auto i = 0; i < ui->widgetChart->legend->itemCount(); ++i)
        {
            ui->widgetChart->graph(i)->setVisible(false);
            ui->widgetChart->legend->item(i)->setTextColor(Qt::GlobalColor::gray);
        }

        ui->widgetChart->replot();
    }
}

void MainWindow::on_chartMouseDoubleClickHandler(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        ui->widgetChart->rescaleAxes(true);
        ui->widgetChart->yAxis->scaleRange(1.20);
    }
    else if (event->button() == Qt::MiddleButton)
    {
        ui->checkBoxAutoTrack->setChecked(!ui->checkBoxAutoTrack->isChecked());
    }

    ui->widgetChart->replot();
}

void MainWindow::on_chartSelectionChanged()
{
    // synchronize selection of graphs with selection of corresponding legend items:
    for (int i = 0; i < ui->widgetChart->graphCount(); ++i)
    {
        QCPGraph *selectedGraph = ui->widgetChart->graph(i);
        QCPPlottableLegendItem *selectedLegendItem = ui->widgetChart->legend->itemWithPlottable(selectedGraph);

        if (selectedLegendItem->selected() || selectedGraph->selected())
        {
            selectedLegendItem->setSelected(true);
            selectedGraph->setSelection(QCPDataSelection(selectedGraph->data()->dataRange()));
            return;
        }
    }
}

void MainWindow::on_tracerShowPointValue(QMouseEvent *event)
{
    if (ui->widgetChart->graphCount() < 1)
        return;

    // Get selected graph (in my case selected means the plot is selected from the legend)
    QCPGraph *graph = nullptr;
    for (auto i = 0; i < ui->widgetChart->graphCount(); ++i)
    {
        if (ui->widgetChart->graph(i)->selected())
        {
            graph = ui->widgetChart->graph(i);
            break;
        }
    }

    if (graph == nullptr)
        return;

    // Setup the item tracer
    this->phaseTracer->setGraph(graph);
    this->phaseTracer->setGraphKey(ui->widgetChart->xAxis->pixelToCoord(event->pos().x()));
    ui->widgetChart->replot();

    // **********Get the values from the item tracer's coords***********
    QPointF temp = this->phaseTracer->position->coords();

    // Show a tooltip which tells the values
    QToolTip::showText(event->globalPos(),
                       tr("<h4>%L1</h4>"
                          "<table>"
                          "<tr>"
                          "<td>X: %L2</td>"
                          "<td>  ,  </td>"
                          "<td>Y: %L3</td>"
                          "</tr>"
                          "</table>")
                       .arg(graph->name())
                       .arg(QTime::fromMSecsSinceStartOfDay(temp.x() * 1000).toString("hh:mm:ss:zzz"))
                       .arg(QString::number(temp.y(), 'f', 5)),
                       ui->widgetChart, ui->widgetChart->rect());
}

// Activates rect mode on middle button press
void MainWindow::on_chartMousePressHandler(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton)
        ui->widgetChart->setSelectionRectMode(QCP::srmZoom);
    else
        ui->widgetChart->setSelectionRectMode(QCP::srmNone);

    ui->widgetChart->replot();
}

void MainWindow::on_chartMouseMoveHandler(QMouseEvent *event)
{
    static int distance = 0, lastPos = 0;

    if (event->buttons() & Qt::LeftButton)
    {
        distance += event->pos().x() - lastPos;
        lastPos = event->pos().x();

        //    qDebug() << "distance: " + QString::number(distance);

        if (abs(distance) > 200)
        {
            ui->checkBoxAutoTrack->setChecked(false);
        }
    }
    else
    {
        distance = 0;
    }
}

void MainWindow::on_updateSerialDeviceList()
{
    QList<QSerialPortInfo> devices = serial.getAvailiblePorts();
    QList<QString> portNames;
    static QList<QString> portNamesOld;

    foreach (auto item, devices)
        portNames.append(item.portName());

    if ((devices.count() >= 1) && (!(portNames.toSet().intersects(portNamesOld.toSet())) || (portNames.count() != portNamesOld.count())))
    {
        ui->comboBoxDevices->clear();

        for (auto i = 0; i < devices.count(); ++i)
        {
            ui->comboBoxDevices->addItem("(" + devices.at(i).portName() + ")" + " " + devices.at(i).description());
        }
    }
    else if ((devices.count() < 1) && !ui->comboBoxDevices->itemText(0).startsWith("No COM devices"))
    {
        ui->comboBoxDevices->clear();
        ui->comboBoxDevices->addItem("No serial devices detected :(");
        ui->comboBoxDevices->setCurrentIndex(ui->comboBoxDevices->count() - 1);
    }

    portNamesOld = portNames;

    this->radioButtonTimer->start(RADIO_BUTTON_UPDATE_SERIAL_DEVICES_ON_INTERVAL);
    ui->radioButtonDeviceUpdate->setChecked(true);
}

void MainWindow::on_pushButtonRefresh_clicked()
{
    qDebug() << "Refreshing serial device list...";
    this->addLog("App >>\t Searching for COM ports...", true);
    this->on_updateSerialDeviceList();
}

void MainWindow::addLog(QString text, bool appendAsLine)
{
    if (ui->pushButtonTextLogToggle->isChecked() == false)
    {
        QString currentDateTime = QDateTime::currentDateTime().toString("hh:mm:ss:zzz ");

        if (ui->checkBoxShowTime->isChecked())
            text = currentDateTime + text;

        // Replace control chars
        if (ui->checkBoxShowControlChars->isChecked())
            text = controlCharactersVisibleConvert(text);

        if (!appendAsLine)
        {
            int sliderPosVertical = ui->textBrowserLogs->verticalScrollBar()->value();
            int sliderPosHorizontal = ui->textBrowserLogs->horizontalScrollBar()->value();

            ui->textBrowserLogs->moveCursor(QTextCursor::MoveOperation::End, QTextCursor::MoveMode::MoveAnchor);
            ui->textBrowserLogs->insertPlainText(text);

            ui->textBrowserLogs->horizontalScrollBar()->setValue(sliderPosHorizontal);

            if (!ui->checkBoxScrollToButtom->isChecked())
                ui->textBrowserLogs->verticalScrollBar()->setValue(sliderPosVertical);
            else
                ui->textBrowserLogs->verticalScrollBar()->setValue(ui->textBrowserLogs->verticalScrollBar()->maximum());
        }
        else
        {
            ui->textBrowserLogs->appendPlainText(text);
        }
    }
}

QString MainWindow::controlCharactersVisibleConvert(QString text)
{
    if (text.replace("\r\n", "\\r\\n\r\n").isEmpty())
    {
        text.replace("\r", "\\r\r");
        text.replace("\n", "\\n\n");
    }

    text.replace("\t", "\\t\t");
    text.replace(' ', QChar(183));

    return text;
}

void MainWindow::addLogBytes(QByteArray bytes, bool hexToBinary, bool appendAsLine)
{
    if (ui->pushButtonTextLogToggle->isChecked() == false)
    {
        QString currentDateTime = QDateTime::currentDateTime().toString("hh:mm:ss:zzz ");

        QString bytesText;

        if (hexToBinary == false)
            bytesText = bytes.toHex(' ');
        else
        {
            for (auto i = 0; i < bytes.size(); ++i)
            {
                bytesText.append(QString::number(bytes[i], 2) + ' ');
            }
        }

        if (ui->checkBoxShowTime->isChecked())
            bytesText = currentDateTime + bytesText;

        if (!appendAsLine)
        {
            int sliderPosVertical = ui->textBrowserLogs->verticalScrollBar()->value();
            int sliderPosHorizontal = ui->textBrowserLogs->horizontalScrollBar()->value();

            ui->textBrowserLogs->moveCursor(QTextCursor::MoveOperation::End, QTextCursor::MoveMode::MoveAnchor);
            ui->textBrowserLogs->insertPlainText(bytesText);

            ui->textBrowserLogs->horizontalScrollBar()->setValue(sliderPosHorizontal);

            if (!ui->checkBoxScrollToButtom->isChecked())
                ui->textBrowserLogs->verticalScrollBar()->setValue(sliderPosVertical);
            else
                ui->textBrowserLogs->verticalScrollBar()->setValue(ui->textBrowserLogs->verticalScrollBar()->maximum());
        }
        else
        {
            ui->textBrowserLogs->appendPlainText(bytesText);
        }
    }
}

void MainWindow::processLogWrite(QString rawLine, QStringList labelList, QList<double> dataList, QList<long> timeList)
{
    if (ui->pushButtonLogging->isChecked()) // Write log into file
    {
        if (ui->comboBoxLogFormat->currentIndex() == 1)
        {
            if (ui->comboBoxLoggingMode->currentIndex() == 0)
                fileLogger.writeLogTXTLine(rawLine, ui->checkBoxSimplifyLog->isChecked());
            else if (ui->comboBoxLoggingMode->currentIndex() == 1)
                fileLogger.writeLogTXTParsedData(labelList, dataList);
        }
        else if (ui->comboBoxLogFormat->currentIndex() == 0)
        {
            fileLogger.writeLogCSV(labelList, dataList);
        }
    }
}

void MainWindow::on_processSerial()
{
    QString serialInput = serial.getString();
    serial.clearAll();

    if (ui->comboBoxTextProcessing->currentIndex() == 1) // Append text to textBrowser
        serialInput = serialInput.trimmed();
    else if (ui->comboBoxTextProcessing->currentIndex() == 2)
        serialInput = serialInput.simplified();

    if (ui->comboBoxFormat->currentIndex() == 0 && serialInput.isEmpty() == false)
    {
        addLog(serialInput, ui->comboBoxAddTextMode->currentIndex());
    }
    else if (ui->comboBoxFormat->currentIndex() == 1 && serialInput.length() > 0)
    {
        addLogBytes(serialInput.toUtf8(), false, ui->comboBoxAddTextMode->currentIndex());
    }
    else if (ui->comboBoxFormat->currentIndex() == 2 && serialInput.length() > 0)
    {
        addLogBytes(serialInput.toUtf8(), true, ui->comboBoxAddTextMode->currentIndex());
    }

    if (serialInput.isEmpty() == false)
    {
        parser.parse(serialInput, ui->comboBoxClockSource->currentIndex() == 0, ui->comboBoxClockSource->currentIndex() == 1, ui->lineEditExternalClockLabel->text()); // Parse string - split into labels + numeric data
        QStringList labelList = parser.getStringListLabels();
        QList<double> numericDataList = parser.getListNumericValues();
        QList<long> timeStamps = parser.getListTimeStamp();

        this->processChart(labelList, numericDataList, timeStamps);
        this->processLogWrite(serialInput, labelList, numericDataList, timeStamps);
        this->saveToRAM(labelList, numericDataList, timeStamps, ui->comboBoxRAMSaveMode->currentIndex(), serialInput);
        this->processTable(labelList, numericDataList);
        this->processLogTable(timeStamps, labelList, numericDataList);

//        QFuture<void> future = QtConcurrent::run([=]() // Cool !
//        {

//        });
    }
}

void MainWindow::clearGraphData(bool replot)
{
    qDebug() << "clear graph data!";
    for (auto i = 0; i < ui->widgetChart->graphCount(); ++i)
    {
        ui->widgetChart->graph(i)->data().data()->clear();
    }

    if (replot)
        ui->widgetChart->replot();
}

void MainWindow::on_processUDP()
{
    QString udpInput = networkUDP.readString();
    networkUDP.clearAll();

    if (ui->comboBoxTextProcessing->currentIndex() == 1)
        udpInput = udpInput.trimmed();
    else if (ui->comboBoxTextProcessing->currentIndex() == 2)
        udpInput = udpInput.simplified();

    if (ui->comboBoxFormat->currentIndex() == 0 && udpInput.isEmpty() == false)
    {
        addLog(udpInput);
    }
    else if (ui->comboBoxFormat->currentIndex() == 1 && udpInput.length() > 0)
    {
        addLogBytes(udpInput.toUtf8(), false, ui->comboBoxAddTextMode->currentIndex());
    }
    else if (ui->comboBoxFormat->currentIndex() == 2 && udpInput.length() > 0)
    {
        addLogBytes(udpInput.toUtf8(), true, ui->comboBoxAddTextMode->currentIndex());
    }

    if (udpInput.isEmpty() == false)
    {
        parser.parse(udpInput, ui->comboBoxClockSource->currentIndex() == 0, ui->comboBoxClockSource->currentIndex() == 1, ui->lineEditExternalClockLabel->text()); // Parse string - split into labels + numeric data
        QStringList labelList = parser.getStringListLabels();
        QList<double> numericDataList = parser.getListNumericValues();
        QList<long> timeStamps = parser.getListTimeStamp();

        this->processLogWrite(udpInput, labelList, numericDataList, timeStamps);
        this->processChart(labelList, numericDataList, timeStamps);
        this->saveToRAM(labelList, numericDataList, timeStamps, ui->comboBoxRAMSaveMode->currentIndex(), udpInput);
        this->processTable(labelList, numericDataList); // Fill tableWidget
        this->processLogTable(timeStamps, labelList, numericDataList);
    }
}

void MainWindow::sendUDPDatagram(QString message)
{
    if (!networkUDP.isOpen())
    {
        addLog("App >>\t Unable to send - port closed.", true);
        return;
    }

    if (ui->comboBoxUDPSendMode->currentText().contains("Broadcast", Qt::CaseSensitivity::CaseInsensitive))
    {
        networkUDP.write(message, QHostAddress::Broadcast, ui->spinBoxUDPTargetPort->value());
    }
    else if (ui->comboBoxUDPSendMode->currentText().contains("LocalHost", Qt::CaseSensitivity::CaseInsensitive))
    {
        networkUDP.write(message, QHostAddress::LocalHost, ui->spinBoxUDPTargetPort->value());
    }
    else
    {
        networkUDP.write(message, QHostAddress(ui->lineEditUDPTargetIP->text()), ui->spinBoxUDPTargetPort->value());
    }

    // addLog("UDP >>\t" + message);
}

void MainWindow::processChart(QStringList labelList, QList<double> numericDataList, QList<long> timeStampsList)
{
    if (timeStampsList.count() == 0 || labelList.count() == 0 || numericDataList.count() == 0 || ui->pushButtonEnablePlot->isChecked())
        return;

    // Execute plotter

    foreach (auto label, labelList)
    {
        bool canAddGraph = true;
        if (ui->widgetChart->graphCount() > 0)
        {
            for (auto i = 0; i < ui->widgetChart->graphCount(); ++i)
            {
                if (ui->widgetChart->graph(i)->name() == label)
                {
                    canAddGraph = false;
                    break;
                }
            }
        }

        if (canAddGraph && ui->widgetChart->graphCount() < ui->spinBoxMaxGraphs->value() &&

                ((ui->comboBoxGraphDisplayMode->currentIndex() == 0) ||

                 (ui->comboBoxGraphDisplayMode->currentIndex() == 1 &&
                  ui->lineEditCustomParsingRules->text().simplified().contains(label, Qt::CaseSensitivity::CaseSensitive)) ||

                 (ui->comboBoxGraphDisplayMode->currentIndex() == 2 &&
                  !ui->lineEditCustomParsingRules->text().simplified().contains(label, Qt::CaseSensitivity::CaseSensitive))))
        {
            ui->widgetChart->addGraph();
            ui->widgetChart->graph()->setName(label);

            ui->widgetChart->graph()->selectionDecorator()->setPen(QPen(Qt::black, 1.0, Qt::PenStyle::DashLine));
            while (ui->widgetChart->graph()->lineStyle() == (QCPGraph::LineStyle::lsImpulse))  //
                ui->widgetChart->graph()->setLineStyle((QCPGraph::LineStyle)(rand() % 5 + 1)); // random line except for impulse
            while (ui->widgetChart->graph()->scatterStyle().shape() == QCPScatterStyle::ScatterShape::ssNone)
                ui->widgetChart->graph()->setScatterStyle(QCPScatterStyle((QCPScatterStyle::ScatterShape)(rand() % 14 + 1))); // random scatter

            int hue = 0; // start with red
            for (auto i = 0; i < ui->widgetChart->graphCount(); ++i)
            {
                ui->widgetChart->graph(i)->setPen(QPen(QColor::fromHsv(hue, 255, 200))); // random color
                hue += 360 / ui->widgetChart->graphCount();
            }

            missingCount.append(0); // Add missing counter to the list (corresponds to each graph index)
        }
    }

    for (auto i = 0; i < ui->widgetChart->graphCount(); ++i)
    {
        for (auto j = 0; j < labelList.count(); ++j)
        {
            if (labelList[j] == ui->widgetChart->graph(i)->name()) // If label matches the graphs name - we can start add points to it
            {
                if (timeStampsList[j] > 0) // if == 0 then we assume that no external clock was parsed propely
                    ui->widgetChart->graph(i)->addData(timeStampsList[j] / 1000.0, numericDataList[j]);

                // break; // no break - slower but if we keep checking for duplicates, we can add multiple points at once
            }
        }

        if (ui->spinBoxMaxTimeRange->value() > 0)
            ui->widgetChart->graph(i)->data().data()->removeBefore((timeStampsList.last() / 1000.0) - ui->spinBoxMaxTimeRange->value()); // Remove old points

        if (ui->spinBoxRemoveOldLabels->value() > 0)
        {
            if (labelList.contains(ui->widgetChart->graph(i)->name()))
                missingCount[i] = 0;
            else
                missingCount[i]++;

            if (missingCount[i] > ui->spinBoxRemoveOldLabels->value())
            {
                ui->widgetChart->removeGraph(i);
                missingCount.removeAt(i); // Remove entry for this label
            }
        }
    }

    ui->widgetChart->replot();
}

void MainWindow::on_setSelectedLabels(QList<QString> *labels, bool customRules)
{
    qDebug() << "on_setSelectedLabels";

    if (!customRules)
    {
        if (labels->count() < 1)
        {
            on_showAllGraphs();
            ui->lineEditCustomParsingRules->clear();
        }
        else
        {
            on_hideAllGraphs();

            if (ui->widgetChart->graphCount() > 0)
            {
                for (auto i = 0; i < ui->widgetChart->graphCount(); ++i)
                {
                    if (labels->contains(ui->widgetChart->graph(i)->name()))
                    {
                        ui->widgetChart->legend->item(i)->setSelected(true);
                        on_showSelectedGraph();
                    }
                }
            }
        }
    }
    else
    {
        ui->comboBoxGraphDisplayMode->setCurrentIndex(1);
    }

    QString labelsToAppend;
    foreach (QString item, *labels)
        labelsToAppend.append(item + " ");

    ui->lineEditCustomParsingRules->setText(labelsToAppend);
    emit on_lineEditCustomParsingRules_editingFinished();
}

void MainWindow::clearGraphs(bool replot)
{
    ui->widgetChart->clearGraphs();

    if (replot)
        ui->widgetChart->replot();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (ui->textBrowserLogs->hasFocus())
    {
        if (event->key() == Qt::Key_Alt)
        {
            qDebug() << "Scrolling to buttom !" + QString::number(event->key());

            ui->textBrowserLogs->verticalScrollBar()->setValue(ui->textBrowserLogs->verticalScrollBar()->maximum());
        }
        else if (ui->checkBoxSendKey->isChecked())
        {
            sendMessageKeyEvent(event);

            if (ui->comboBoxMessagesDisplayMode->currentIndex() == 0)
                addLog(">>\n\t" + event->text(), ui->comboBoxAddTextMode->currentIndex());
        }
    }
    else if (ui->widgetChart->hasFocus())
    {
        if (event->key() == Qt::Key_T)
        {
            ui->checkBoxEnableTracer->toggle();
        }
        else if (event->key() == Qt::Key_X)
        {
            ui->checkBoxAutoTrack->toggle();
        }
        else if (event->key() == Qt::Key_Y)
        {
            ui->checkBoxAutoRescaleY->toggle();
        }
        else if (event->key() == Qt::Key_L)
        {
            ui->checkBoxShowLegend->toggle();
        }
    }

    if (event->key() == Qt::Key::Key_F1)
    {
        QWhatsThis::enterWhatsThisMode();
    }
}

void MainWindow::sendMessageKeyEvent(QKeyEvent *event)
{
    if (ui->tabWidgetControlSection->currentIndex() == 0)
        sendSerial(event->text());
    else if (ui->tabWidgetControlSection->currentIndex() == 1)
        sendUDPDatagram(event->text());
    else
    {
        sendSerial(event->text());
        sendUDPDatagram(event->text());
    }
}

void MainWindow::sendSerial(QString message) // TODO - move to serial - error via signal/slot
{
    if (!serial.send(message))
        this->addLog("App >>\t Unable to send! Serial port closed !", true);
}

void MainWindow::saveToRAM(QStringList newlabelList, QList<double> newDataList, QList<long> newTimeList, bool saveText, QString text)
{
    if (ui->checkBoxAutoSaveBuffer->isChecked() == false)
        return;

    if (text.isEmpty() == false && saveText == true)
        parser.appendSetToMemory(newlabelList, newDataList, newTimeList, text);
    else
        parser.appendSetToMemory(newlabelList, newDataList, newTimeList);
}

void MainWindow::loadFromRAM(bool loadText)
{
    QStringList RAMLabels = parser.getLabelStorage();
    QList<double> RAMData = parser.getDataStorage();
    QList<long> RAMTime = parser.getTimeStorage();

    if (loadText)
    {
        QStringList RAMText = parser.getTextList();
        foreach (auto line, RAMText)
            addLog("Mem >>\t" + line);
    }

    if (RAMLabels.isEmpty() || RAMData.isEmpty() || RAMTime.isEmpty())
        return;

    this->processChart(RAMLabels, RAMData, RAMTime);
    this->processTable(RAMLabels,RAMData);
    this->processLogTable(RAMTime, RAMLabels, RAMData);
}

void MainWindow::on_checkBoxAutoRefresh_toggled(bool checked)
{
    if (checked == true)
    {
        ui->pushButtonRefresh->setEnabled(false);
        serialDeviceCheckTimer->start(SERIAL_DEVICE_CHECK_TIMER_INTERVAL);
    }
    else
    {
        ui->pushButtonRefresh->setEnabled(true);
        serialDeviceCheckTimer->stop();
    }
}

void MainWindow::on_pushButtonClear_clicked()
{
    ui->textBrowserLogs->clear();
}

void MainWindow::on_lineEditHighlight_returnPressed()
{
    on_highlighLog(ui->lineEditHighlight->text());
}

void MainWindow::on_highlighLog(QString searchString)
{
    QTextDocument *document = ui->textBrowserLogs->document();
    QTextCursor highlightCursor(document);

    // clear formatting !
    highlightCursor.setPosition(document->characterCount() - 1, QTextCursor::KeepAnchor);
    highlightCursor.mergeCharFormat(QTextCharFormat());
    highlightCursor.setPosition(0, QTextCursor::MoveAnchor);

    if (!searchString.isEmpty())
    {
        QTextCharFormat colorFormat = highlightCursor.charFormat();

        colorFormat.setFontLetterSpacing(110);
        colorFormat.setForeground(Qt::black);
        colorFormat.setFontWeight(QFont::Bold);
        colorFormat.setBackground(Qt::gray);

        while (!highlightCursor.isNull() && !highlightCursor.atEnd())
        {
            highlightCursor = document->find(searchString, highlightCursor);

            if (!highlightCursor.isNull())
            {
                highlightCursor.movePosition(QTextCursor::NextCell, QTextCursor::KeepAnchor);
                highlightCursor.mergeCharFormat(colorFormat);
            }
        }
    }
}

void MainWindow::on_checkBoxWrapText_toggled(bool checked)
{
    if (checked)
        ui->textBrowserLogs->setLineWrapMode(QPlainTextEdit::LineWrapMode::WidgetWidth);
    else
        ui->textBrowserLogs->setLineWrapMode(QPlainTextEdit::LineWrapMode::NoWrap);
}

void MainWindow::on_checkBoxEnableTracer_toggled(bool checked)
{
    if (checked)
    {
        createChartTracer();
        connect(ui->widgetChart, SIGNAL(mouseMove(QMouseEvent *)), this, SLOT(on_tracerShowPointValue(QMouseEvent *)));
    }
    else
    {
        disconnect(ui->widgetChart, SIGNAL(mouseMove(QMouseEvent *)), this, SLOT(on_tracerShowPointValue(QMouseEvent *)));
        delete phaseTracer;
        phaseTracer = nullptr;
    }

    ui->widgetChart->replot();
}

void MainWindow::createChartTracer()
{
    // add the phase tracer (red circle) which sticks to the graph data:
    this->phaseTracer = new QCPItemTracer(ui->widgetChart);
    this->phaseTracer->setInterpolating(false);

    if (ui->comboBoxTracerStyle->currentText().contains("Crosshair", Qt::CaseSensitivity::CaseInsensitive))
        this->phaseTracer->setStyle(QCPItemTracer::tsCrosshair);
    else
        this->phaseTracer->setStyle(QCPItemTracer::tsCircle);

    this->phaseTracer->setPen(QPen(Qt::red));
    this->phaseTracer->setBrush(Qt::red);
    this->phaseTracer->setSize(8);
}

void MainWindow::on_pushButtonClearAll_clicked()
{
    ui->widgetChart->xAxis->setRange(-1.0, ui->spinBoxScrollingTimeRange->value());
    ui->widgetChart->yAxis->setRange(-1.0, 1.0);

    parser.restartChartTimer();
    parser.clearExternalClock();
    clearGraphs(true);

    ui->tableWidgetParsedData->clearContents();
    ui->tableWidgetParsedData->setRowCount(0);
    ui->tableWidgetLogTable->setRowCount(0);
    ui->tableWidgetLogTable->setColumnCount(1);

    if (ui->checkBoxRAMClearChart->isChecked())
        parser.clearStorage();
}

void MainWindow::on_pushButtonEnablePlot_toggled(bool checked)
{
    if (checked)
        ui->pushButtonEnablePlot->setText("Enable");
    else
        ui->pushButtonEnablePlot->setText("Disable");
}

void MainWindow::on_pushButtonSend_clicked()
{
    if (ui->comboBoxSend->findText(ui->comboBoxSend->currentText()) < 0)
        ui->comboBoxSend->addItem(ui->comboBoxSend->currentText()); // add to history (here we have to do it manually)

    on_comboBoxSendReturnPressedSlot();
}

void MainWindow::on_pushButtonClearHistory_clicked()
{
    ui->comboBoxSend->clear();
}

void MainWindow::on_pushButtonClearGraphs_clicked()
{
    clearGraphData(true);
}

void MainWindow::on_lineEditCustomParsingRules_editingFinished()
{
    ui->lineEditCustomParsingRules->setText(ui->lineEditCustomParsingRules->text().replace(" || ", " "));
    ui->lineEditCustomParsingRules->setText(ui->lineEditCustomParsingRules->text().simplified().replace(" ", " || "));
    clearGraphs(true);
    loadFromRAM(false);
}

void MainWindow::on_spinBoxMaxGraphs_valueChanged(int arg1)
{
    QPalette *paletteRed = new QPalette();
    paletteRed->setColor(QPalette::Text, Qt::GlobalColor::red);

    if (arg1 > 10)
        ui->spinBoxMaxGraphs->setPalette(*paletteRed);
    else
        ui->spinBoxMaxGraphs->setPalette(QPalette());

    this->clearGraphs(false);
    this->loadFromRAM(false);

    ui->widgetChart->replot();
}

void MainWindow::on_spinBoxMaxLines_valueChanged(int arg1)
{
    ui->textBrowserLogs->document()->setMaximumBlockCount(arg1);
}

void MainWindow::on_checkBoxAutoTrack_toggled(bool checked)
{
    if (checked)
    {
        ui->spinBoxScrollingTimeRange->setEnabled(true);
        ui->checkBoxAutoRescaleY->setEnabled(true);
    }
    else
    {
        ui->spinBoxScrollingTimeRange->setEnabled(false);
        ui->checkBoxAutoRescaleY->setEnabled(false);
    }
}

void MainWindow::on_spinBoxProcessingDelay_valueChanged(int arg1)
{
    int newInterval = qMax(1, arg1); // Never 0 ! Couses CPU run like crazy !

    serialStringProcessingTimer->setInterval(newInterval);
    udpStringProcessingTimer->setInterval(newInterval);

    QPalette *paletteRed = new QPalette();
    paletteRed->setColor(QPalette::Text, Qt::GlobalColor::red);

    if (newInterval > ui->spinBoxProcessingDelay->minimum())
        ui->spinBoxProcessingDelay->setPalette(*paletteRed);
    else
        ui->spinBoxProcessingDelay->setPalette(QPalette());
}

void MainWindow::on_comboBoxTracerStyle_currentIndexChanged(const QString &arg1)
{
    if (ui->checkBoxEnableTracer->isChecked())
    {
        if (phaseTracer != nullptr)
        {
            disconnect(ui->widgetChart, SIGNAL(mouseMove(QMouseEvent *)), this, SLOT(on_tracerShowPointValue(QMouseEvent *)));
            delete phaseTracer;
            phaseTracer = nullptr;
        }

        createChartTracer();
        connect(ui->widgetChart, SIGNAL(mouseMove(QMouseEvent *)), this, SLOT(on_tracerShowPointValue(QMouseEvent *)));

        ui->widgetChart->replot();
    }
}

void MainWindow::on_pushButtonSerialConnect_toggled(bool checked)
{
    if (checked)
    {
        if (serial.getAvailiblePorts().count() < 1)
        {
            addLog("App >>\t No devices available", true);
            addLog("App >>\t Unable to open serial port!", true);
            ui->pushButtonSerialConnect->setChecked(false);
            return;
        }

        QString parsedPortName = QSerialPortInfo::availablePorts().at(ui->comboBoxDevices->currentIndex()).portName();
        qint32 parsedBaudRate = ui->comboBoxBaudRates->currentText().toInt();
        QString dataBits = ui->comboBoxDataBits->currentText();
        QString stopBits = ui->comboBoxStopBits->currentText();
        QString parity = ui->comboBoxParity->currentText();
        QString flowControl = ui->comboBoxFlowControl->currentText();

        if (serial.begin(parsedPortName, parsedBaudRate, dataBits, parity, stopBits, flowControl, ui->checkBoxDTR->isChecked()))
        {
            serialStringProcessingTimer->start(ui->spinBoxProcessingDelay->value());
            parser.restartChartTimer();

            if (ui->pushButtonLogging->isChecked() && fileLogger.isOpen() == false) // logger on standby ?
                fileLogger.beginLog(ui->lineEditSaveLogPath->text(), ui->checkBoxAutoLogging->isChecked(), ui->lineEditSaveFileName->text(), ui->checkBoxTruncateFileOnSave->isChecked());

            connect(serialStringProcessingTimer, SIGNAL(timeout()), this, SLOT(on_processSerial()));

            addLog("App >>\t Serial port opened. " + serial.getSerialInfo() + " DTR: " + QString::number(ui->checkBoxDTR->isChecked()), true);
            ui->pushButtonSerialConnect->setText("Disconnect");
        }
        else
        {
            ui->pushButtonSerialConnect->setChecked(false);
            addLog("App >>\t Unable to open serial port!", true);
        }
    }
    else
    {
        serialStringProcessingTimer->stop();

        disconnect(serialStringProcessingTimer, SIGNAL(timeout()), this, SLOT(on_processSerial()));

        if (serial.end())
        {
            addLog("App >>\t Connection closed.", true);
            ui->pushButtonSerialConnect->setText("Connect");
        }
        else
        {
            ui->pushButtonSerialConnect->setChecked(true);
            addLog("App >>\t ERROR: Unable to close cennection !", true);
        }

        if (!ui->pushButtonSerialConnect->isChecked() && !ui->pushButtonUDPConnect->isChecked())
        {
            fileLogger.closeFile();
        }
    }
}

void MainWindow::on_actionSave_As_triggered()
{
    qDebug() << "Click save file !";
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Log"), "", tr("(*.txt);;All Files (*)"));

    if (fileName.isEmpty())
    {
        addLog("App >>\t Saving file aborted - filename not specified.", true);
        return;
    }
    else
    {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly))
        {
            QMessageBox::information(this, tr("Unable to open file"), file.errorString());
            return;
        }

        QTextStream out(&file);
        out << ui->textBrowserLogs->toPlainText();
    }
}

void MainWindow::on_actionFull_chart_triggered()
{
    ui->splitterReceivedData->setSizes(QList<int>({0, this->width()}));
}

void MainWindow::on_actionFull_text_view_triggered()
{
    ui->splitterReceivedData->setSizes(QList<int>({this->width(), 0}));
}

void MainWindow::on_action50_50_view_triggered()
{
    ui->splitterReceivedData->setSizes(QList<int>({this->width() / 2, this->width() / 2}));
}

void MainWindow::on_actionWhat_s_new_triggered()
{
    on_printIntroChangelog();
}

void MainWindow::on_actionQuit_triggered()
{
    this->close();
}

void MainWindow::on_actionPlotter_triggered()
{
    ui->stackedWidgetGraphView->setCurrentIndex(0);
}

void MainWindow::on_action3D_orientation_triggered()
{
    //  ui->stackedWidget->setCurrentIndex(1); // WIP
}

void MainWindow::on_printPlot(QPrinter *printer)
{
    printer->setPageSize(QPrinter::PageSize::A4);
    QCPPainter painter(printer);
    QRectF pageRect = printer->pageRect(QPrinter::DevicePixel);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform, true);
    int plotWidth = ui->widgetChart->viewport().width(), plotHeight = ui->widgetChart->viewport().height();
    double scale = pageRect.width() / (float)plotWidth;

    painter.setMode(QCPPainter::pmNoCaching);
    painter.scale(scale, scale);

    ui->widgetChart->toPainter(&painter, plotWidth, plotHeight);
}

void MainWindow::on_printLog(QPrinter *printer)
{
    ui->textBrowserLogs->print(printer);
}

void MainWindow::on_checkBoxShowLegend_toggled(bool checked)
{
    ui->widgetChart->legend->setVisible(checked);
    ui->widgetChart->replot();
}

void MainWindow::on_actionSave_graph_as_triggered()
{
}

void MainWindow::on_actionPrint_Graph_triggered()
{
    QPrinter chartPrinter;
    chartPrinter.setFullPage(true);

    QPrintPreviewDialog previewDialog(&chartPrinter);
    previewDialog.setWindowFlags(Qt::Window);

    connect(&previewDialog, SIGNAL(paintRequested(QPrinter *)), SLOT(on_printPlot(QPrinter *)));
    previewDialog.exec();
}

void MainWindow::on_actionPrint_log_triggered()
{
    QPrinter logPrinter;

    QPrintPreviewDialog previewLogDialog(&logPrinter);
    previewLogDialog.setWindowFlags(Qt::Window);

    connect(&previewLogDialog, SIGNAL(paintRequested(QPrinter *)), SLOT(on_printLog(QPrinter *)));
    previewLogDialog.exec();
}

void MainWindow::on_pushButtonUDPConnect_toggled(bool checked)
{
    if (checked)
    {
        bool status = false;
        if (ui->comboBoxUDPReceiveMode->currentText().contains("LocalHost"))
        {
            status = networkUDP.begin(QHostAddress::LocalHost, ui->spinBoxUDPReceivePort->value());
        }
        else if (ui->comboBoxUDPReceiveMode->currentText().contains("Any"))
        {
            status = networkUDP.begin(QHostAddress::Any, ui->spinBoxUDPReceivePort->value());
        }
        else if (ui->comboBoxUDPReceiveMode->currentText().contains("SpecialAddress"))
        {
            status = networkUDP.begin(QHostAddress(ui->lineEditUDPTargetIP->text()), ui->spinBoxUDPReceivePort->value());
        }

        if (status == true)
        {
            udpStringProcessingTimer->start(ui->spinBoxProcessingDelay->value());
            parser.restartChartTimer();

            if (ui->pushButtonLogging->isChecked() && fileLogger.isOpen() == false)
                fileLogger.beginLog(ui->lineEditSaveLogPath->text(), ui->checkBoxAutoLogging->isChecked(), ui->lineEditSaveFileName->text(), ui->checkBoxTruncateFileOnSave->isChecked());

            connect(udpStringProcessingTimer, SIGNAL(timeout()), this, SLOT(on_processUDP()));

            addLog("App >>\t UDP port opened.", true);

            ui->pushButtonUDPConnect->setText("Close Connection");
        }
        else
        {
            addLog("App >>\t UDP error. Unable to bind", true);
        }
    }
    else
    {
        if (networkUDP.end())
        {
            udpStringProcessingTimer->stop();

            addLog("App >>\t UDP port closed.", true);

            disconnect(udpStringProcessingTimer, SIGNAL(timeout()), this, SLOT(on_processUDP()));

            if (!ui->pushButtonSerialConnect->isChecked() && !ui->pushButtonUDPConnect->isChecked())
                fileLogger.closeFile();

            ui->pushButtonUDPConnect->setText("Open Connection");
        }
    }
}

void MainWindow::on_comboBoxUDPSendMode_currentIndexChanged(const QString &arg1)
{
    if (arg1.contains("SpecialAddress") || ui->comboBoxUDPReceiveMode->currentText().contains("SpecialAddress"))
        ui->lineEditUDPTargetIP->setEnabled(true);
    else
        ui->lineEditUDPTargetIP->setEnabled(false);
}

void MainWindow::on_comboBoxUDPReceiveMode_currentIndexChanged(const QString &arg1)
{
    if (ui->pushButtonUDPConnect->isChecked() == true)
    {
        if (ui->comboBoxUDPReceiveMode->currentText().contains("LocalHost"))
        {
            networkUDP.bind(QHostAddress::LocalHost, ui->spinBoxUDPReceivePort->value());
        }
        else if (ui->comboBoxUDPReceiveMode->currentText().contains("Any"))
        {
            networkUDP.bind(QHostAddress::Any, ui->spinBoxUDPReceivePort->value());
        }
        else if (ui->comboBoxUDPReceiveMode->currentText().contains("SpecialAddress"))
        {
            networkUDP.bind(QHostAddress(ui->lineEditUDPTargetIP->text()), ui->spinBoxUDPReceivePort->value());
        }
    }

    if (arg1.contains("SpecialAddress") || ui->comboBoxUDPSendMode->currentText().contains("SpecialAddress"))
        ui->lineEditUDPTargetIP->setEnabled(true);
    else
        ui->lineEditUDPTargetIP->setEnabled(false);
}

void MainWindow::on_pushButtonLogging_toggled(bool checked)
{
    if (checked)
    {
        if (ui->checkBoxAutoLogging->isChecked() == false && ui->lineEditSaveFileName->text().isEmpty())
        {
            addLog("App >>\t logger error - filename not specified !", true);
            ui->pushButtonLogging->setChecked(false);
            return;
        }

        if (ui->pushButtonSerialConnect->isChecked() || ui->pushButtonUDPConnect->isChecked())
        {
            if (!fileLogger.beginLog(ui->lineEditSaveLogPath->text(), ui->checkBoxAutoLogging->isChecked(), ui->lineEditSaveFileName->text()), ui->checkBoxTruncateFileOnSave->isChecked())
            {
                addLog("App >>\t logger error - unable to open File", true);
                ui->pushButtonLogging->setChecked(false);
                return;
            }
        }

        ui->comboBoxLogFormat->setEnabled(false);
        ui->pushButtonLogging->setText("Disable Logging");
    }
    else
    {
        ui->comboBoxLogFormat->setEnabled(true);
        ui->pushButtonLogging->setText("Enable Logging");
        fileLogger.closeFile();
    }
}

void MainWindow::on_checkBoxAutoLogging_toggled(bool checked)
{
    if (checked)
    {
        ui->pushButtonLogging->setEnabled(false);
        ui->lineEditSaveFileName->setEnabled(false);
        ui->pushButtonAddDateTime->setEnabled(false);
        ui->comboBoxLogFormat->setEnabled(false);

        ui->lineEditSaveFileName->setText("%DateTime%_Log" + ui->comboBoxLogFormat->currentText());

        ui->pushButtonLogging->setChecked(true); // Trigger begin log
        ui->pushButtonLogging->setText("Auto");
    }
    else
    {
        ui->pushButtonLogging->setEnabled(true);
        ui->lineEditSaveFileName->setEnabled(true);
        ui->pushButtonAddDateTime->setEnabled(true);

        ui->pushButtonLogging->setChecked(false);
        ui->pushButtonLogging->setText("Enable Logging");

        ui->lineEditSaveFileName->clear();
        ui->comboBoxLogFormat->setEnabled(true);
    }
}

void MainWindow::on_pushButtonSetDefaultLogPath_clicked()
{
    ui->lineEditSaveLogPath->setText(qApp->applicationDirPath() + "/Logs");
}

void MainWindow::on_pushButtonAddDateTime_clicked()
{
    ui->lineEditSaveFileName->setText(QDateTime::currentDateTime().toString("dd.MM.yyyy_hh:mm:ss:zzz_") + ui->lineEditSaveFileName->text());
}

void MainWindow::on_pushButtonLogPath_clicked()
{
    QString newPath;
    if (QDir(ui->lineEditSaveLogPath->text()).isReadable())
        newPath = QFileDialog::getExistingDirectory(this, "Set log path:", ui->lineEditSaveLogPath->text());
    else
        newPath = QFileDialog::getExistingDirectory(this, "Set log path:", QDir::homePath());

    if (newPath.isEmpty() == false)
        ui->lineEditSaveLogPath->setText(newPath);
}

void MainWindow::on_comboBoxSerialReadMode_currentIndexChanged(int index)
{
    serial.setReadMode(index);
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    qApp->aboutQt();
}

void MainWindow::on_pushButtonClearTable_clicked()
{
    ui->tableWidgetParsedData->clearContents();
    ui->tableWidgetParsedData->setRowCount(0);
}

void MainWindow::on_tableWidgetParsedData_customContextMenuRequested(const QPoint &pos)
{
    QMenu *menu = new QMenu(this);
    menu->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose);

    menu->addAction("Apply to custom display", this, [=]() { emit on_pushButtonSetSelectedToGraph_clicked(); }); // C++ lambda expression
    menu->addAction("Copy as text", this, [=]() {
        QString copyText;
        foreach (auto item, ui->tableWidgetParsedData->selectedItems())
            copyText.append(item->text() + " ");
        QApplication::clipboard()->setText(copyText);
    }); // C++ lambda expression

    menu->addAction("Set as clock label", this, [=]() { ui->lineEditExternalClockLabel->setText(ui->tableWidgetParsedData->selectedItems().first()->text()); }); // C++ lambda expression
    menu->addAction("Clear table contents", this, [=]() { ui->tableWidgetParsedData->clearContents(); ui->tableWidgetParsedData->setRowCount(0); });                                                                                                    // C++ lambda expression

    menu->popup(ui->tableWidgetParsedData->mapToGlobal(pos));
}

void MainWindow::on_pushButtonSetSelectedToGraph_clicked()
{
    QStringList tableLabels;
    for (auto i = 0; i < ui->tableWidgetParsedData->rowCount(); ++i)
    {
        if (ui->tableWidgetParsedData->item(i, 0)->isSelected())
            tableLabels.append(ui->tableWidgetParsedData->item(i, 0)->text());
    }

    // clearGraphs(true);

    if (ui->spinBoxMaxGraphs->value() < tableLabels.count())
        ui->spinBoxMaxGraphs->setValue(tableLabels.count());

    on_setSelectedLabels(&tableLabels);
}

void MainWindow::on_toolButtonAdvancedGraphMenu_clicked()
{
    if (ui->toolButtonAdvancedGraphMenu->arrowType() == Qt::ArrowType::DownArrow)
    {
        ui->splitterGraphTable->setSizes({ui->groupBoxLog->height(), 0});
        ui->toolButtonAdvancedGraphMenu->setArrowType(Qt::ArrowType::UpArrow);
    }
    else
    {
        ui->splitterGraphTable->setSizes({static_cast<int>(ui->groupBoxLog->height() * 0.75), static_cast<int>(ui->groupBoxLog->height() * 0.25)});
        ui->toolButtonAdvancedGraphMenu->setArrowType(Qt::ArrowType::DownArrow);
    }
}

void MainWindow::on_splitterGraphTable_splitterMoved(int pos, int index)
{
    if (pos < ui->splitterGraphTable->height() - 30)
        ui->toolButtonAdvancedGraphMenu->setArrowType(Qt::ArrowType::DownArrow);
    else
        ui->toolButtonAdvancedGraphMenu->setArrowType(Qt::ArrowType::UpArrow);
}

void MainWindow::on_pushButtonTextLogToggle_toggled(bool checked)
{
    if (checked)
    {
        ui->pushButtonTextLogToggle->setText("Enable");
    }
    else
    {
        ui->pushButtonTextLogToggle->setText("Disable");
    }
}

void MainWindow::on_lineEditExternalClockLabel_editingFinished()
{
    ui->lineEditExternalClockLabel->setText(ui->lineEditExternalClockLabel->text().simplified());
}

void MainWindow::on_pushButtonLoadPath_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open Log File:"), "",
                                                    tr("(*.txt *.csv);;All Files (*)"));

    ui->lineEditLoadFilePath->setText(fileName);
    QFile file(fileName);

    if (!file.exists())
    {
        ui->lineEditFileInfo->setText("File doesnt exist !");
        return;
    }

    QList<QTime> timeRange = fileReader.getFileTimeRange(&file);
    ui->timeEditMinParsingTime->setTime(timeRange.first());
    ui->timeEditMaxParsingTime->setTime(timeRange.last());

    QString info = "Size: " + QString::number(file.size());
    ui->lineEditFileInfo->setText(info);
}

void MainWindow::on_processLoadedFileLine(QString *line, int *progressPercent)
{
    parser.parse(*line, true, true, ""); // Parse string - split into labels + numeric data
    QStringList labelList = parser.getStringListLabels();
    QList<double> numericDataList = parser.getListNumericValues();
    QList<long> timeStampList = parser.getListTimeStamp();

    this->processChart(labelList, numericDataList, timeStampList);
    this->processTable(labelList, numericDataList); // Fill tableWidget
    // this->saveToRAM(labelList, numericDataList, timeStampList);
    //  addLog(line);

    ui->progressBarLoadFile->setValue(*progressPercent);
}

void MainWindow::on_updateProgressBar(float *percent)
{
    ui->progressBarLoadFile->setValue(*percent);
}

void MainWindow::on_processLoadedFile(QString *text)
{
    disconnect(&fileReader, SIGNAL(textReady(QString *)), this, SLOT(on_processLoadedFile(QString *)));
    connect(&parser, SIGNAL(updateProgress(float *)), this, SLOT(on_updateProgressBar(float *)));

    parser.setReportProgress(true);
    parser.clearStorage();
    parser.setParsingTimeRange(ui->timeEditMinParsingTime->time(), ui->timeEditMaxParsingTime->time());

    // Select file type
    if (ui->comboBoxLogFormat->currentIndex() == 0)
        parser.parseCSV(*text, (bool)ui->comboBoxClockSource->currentIndex() == 1, ui->lineEditExternalClockLabel->text());
    else
        parser.parse(*text, false, true, "");

    // disconnect events, reset parser
    parser.resetTimeRange();
    parser.setReportProgress(false);
    disconnect(&parser, SIGNAL(updateProgress(float *)), this, SLOT(on_updateProgressBar(float *)));

    // Parsing
    {
        QStringList labelList = parser.getStringListLabels();
        QList<double> numericDataList = parser.getListNumericValues();
        QList<long> timeStampList = parser.getListTimeStamp();

        this->processChart(labelList, numericDataList, timeStampList);
        this->processTable(labelList, numericDataList);
        this->saveToRAM(labelList, numericDataList, timeStampList);
    }

    // Add text to log
    {
        if (ui->checkBoxAppendLoadedTextToLog->isChecked() == true)
            this->ui->textBrowserLogs->appendPlainText(*text);
    }
}

void MainWindow::on_pushButtonLoadRAMBuffer_clicked()
{
    if (ui->checkBoxRAMClearChart->isChecked())
        this->clearGraphData(false);

    loadFromRAM((bool)ui->comboBoxRAMLoadMode->currentIndex());
}

void MainWindow::on_pushButtonRAMClear_clicked()
{
    parser.clearStorage();
}

void MainWindow::on_pushButtonLoadFile_clicked()
{
    ui->progressBarLoadFile->setValue(0);

    if (ui->pushButtonLoadFile->text().contains("Load"))
    {
        this->clearGraphs(true);
        ui->pushButtonLoadFile->setText("Cancel");

        if (QDir(ui->lineEditLoadFilePath->text().trimmed()).isReadable())
        {
            QFile inputFile(ui->lineEditLoadFilePath->text().trimmed());
            connect(&fileReader, SIGNAL(textReady(QString *)), this, SLOT(on_processLoadedFile(QString *)));

            connect(&fileReader, &FileReader::fileReadFinished, this, [=]() {
                ui->pushButtonLoadFile->setText("Load File");
                ui->progressBarLoadFile->setValue(100);
            });

            if (fileReader.readAllAtOnce(&inputFile))
            {
                addLog("App >>\t Read file succesfully... ", true);
            }
            else
            {
                addLog("App >>\t Error - invalid file !", true);
                ui->pushButtonLoadFile->setText("Load File");
                ui->progressBarLoadFile->setValue(0);
            }
        }
        else
        {
            addLog("App >>\t Error - invalid file path !", true);
            ui->pushButtonLoadFile->setText("Load File");
            ui->progressBarLoadFile->setValue(0);
        }
    }
    else
    {
        parser.abort();
        disconnect(&fileReader, SIGNAL(textReady(QString *)), this, SLOT(on_processLoadedFile(QString *)));
        ui->pushButtonLoadFile->setText("Load File");
        ui->progressBarLoadFile->setValue(0);
    }
}

void MainWindow::on_lineEditLoadFilePath_customContextMenuRequested(const QPoint &pos)
{
    QMenu *menu = new QMenu(this);
    menu->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose);
    menu->addAction("Open file", this, [=]() { QDesktopServices::openUrl(QUrl(ui->lineEditLoadFilePath->text())); });                                                 // C++ lambda expression
    menu->addAction("Open folder", this, [=]() { QDesktopServices::openUrl(QUrl(QDir(QFileInfo(ui->lineEditLoadFilePath->text()).absoluteDir()).absolutePath())); }); // C++ lambda expression
    menu->popup(ui->lineEditLoadFilePath->mapToGlobal(pos));
}

void MainWindow::on_pushButtonSaveRAMBuffer_clicked()
{
    //QStringList newlabelList, QList<double> newDataList, QList<long> newTimeList, QString text)

    QStringList newlabelList;
    QList<double> newDataList;
    QList<long> newTimeList;
    QString text = "";

    for (auto i = 0; i < ui->widgetChart->graphCount(); ++i)
    {
        newlabelList.append(ui->widgetChart->graph(i)->name());

        QCPGraphDataContainer::const_iterator begin = ui->widgetChart->graph(i)->data()->at(ui->widgetChart->graph(i)->data()->dataRange().begin()); // get range begin iterator from index
        QCPGraphDataContainer::const_iterator end = ui->widgetChart->graph(i)->data()->at(ui->widgetChart->graph(i)->data()->dataRange().end());     // get range end iterator from index

        for (QCPGraphDataContainer::const_iterator it = begin; it != end; ++it)
        {
            newTimeList.append(it->key);
            newDataList.append(it->value);
        }
    }

    if (ui->comboBoxRAMSaveMode->currentIndex() == 1)
        text.append(ui->textBrowserLogs->toPlainText());

    parser.appendSetToMemory(newlabelList, newDataList, newTimeList, text);
}

void MainWindow::on_actionInfo_triggered()
{
    // infoDialog.setModal(true);
    infoDialog.show();
}

void MainWindow::on_comboBoxLoggingMode_currentIndexChanged(int index)
{
    if (index == 0)
        ui->checkBoxSimplifyLog->setEnabled(true);
    else if (index == 1)
        ui->checkBoxSimplifyLog->setEnabled(false);
}

void MainWindow::on_actionWhats_this_triggered()
{
    QWhatsThis::enterWhatsThisMode();
}

void MainWindow::on_lineEditSaveFileName_editingFinished()
{
    if (ui->comboBoxLogFormat->currentText() == ".txt")
    {
        if (ui->lineEditSaveFileName->text().endsWith(".txt") == false)
            ui->lineEditSaveFileName->setText(ui->lineEditSaveFileName->text().append(".txt")); // auto-complete filename with .txt extension
    }
    else if (ui->comboBoxLogFormat->currentText() == ".csv")
    {
        if (ui->lineEditSaveFileName->text().endsWith(".csv") == false)
            ui->lineEditSaveFileName->setText(ui->lineEditSaveFileName->text().append(".csv")); // auto-complete filename with .txt extension
    }
}

void MainWindow::on_checkBoxAutoSaveBuffer_toggled(bool checked)
{
    parser.clearStorage();
    ui->pushButtonSaveParserMemory->setEnabled(!checked);
}

void MainWindow::on_comboBoxClockSource_currentIndexChanged(int index)
{
    if (index == 1)
    {
        ui->lineEditExternalClockLabel->setEnabled(true);
        ui->comboBoxExternalTimeFormat->setEnabled(true);
    }
    else
    {
        ui->lineEditExternalClockLabel->setEnabled(false);
        ui->comboBoxExternalTimeFormat->setEnabled(false);
    }
}

void MainWindow::on_comboBoxFormat_currentIndexChanged(int index)
{
    if (index == 0)
        ui->comboBoxTextProcessing->setEnabled(true);
    else
        ui->comboBoxTextProcessing->setEnabled(false);
}

void MainWindow::on_toolButtonHideTable_clicked()
{
    ui->splitterGraphTable->setSizes({ui->splitterGraphTable->width(), 0});
    ui->toolButtonAdvancedGraphMenu->setArrowType(Qt::ArrowType::UpArrow);
}

void MainWindow::on_comboBoxAddTextMode_currentIndexChanged(int index)
{
    //    if (index == 1)
    //    {
    //        ui->radioButtonScrollToButtom->setCheckable(false);
    //         ui->radioButtonScrollToButtom->setChecked(false);
    //    }
    //    else
    //    {
    //        ui->radioButtonScrollToButtom->setCheckable(true);
    //    }
}

void MainWindow::on_actionHide_parser_data_triggered()
{
    ui->splitterGraphTable->setSizes({ui->splitterGraphTable->width(), 0});
}

void MainWindow::on_actionShow_parser_data_triggered()
{
    ui->splitterGraphTable->setSizes({ui->splitterGraphTable->width(), ui->splitterGraphTable->height() / 2});
}

void MainWindow::on_radioButtonScrollToButtom_clicked()
{
    ui->textBrowserLogs->verticalScrollBar()->setValue(ui->textBrowserLogs->verticalScrollBar()->maximum());
}

void MainWindow::on_comboBoxLogFormat_currentIndexChanged(int index)
{
    ui->comboBoxLoggingMode->setEnabled((bool)index);
    ui->checkBoxSimplifyLog->setEnabled((bool)index);
    // ui->pushButtonLoadFile->setEnabled((bool)index);

    if (ui->comboBoxLogFormat->currentText().contains("txt"))
        ui->lineEditSaveFileName->setText(ui->lineEditSaveFileName->text().replace("csv", "txt"));
    else if (ui->comboBoxLogFormat->currentText().contains("csv"))
        ui->lineEditSaveFileName->setText(ui->lineEditSaveFileName->text().replace("txt", "csv"));
}

void MainWindow::on_comboBoxGraphDisplayMode_currentIndexChanged(int index)
{
    clearGraphs(true);
    loadFromRAM(0);

    QPalette *paletteRed = new QPalette();
    paletteRed->setColor(QPalette::Text, Qt::GlobalColor::red);

    if (index == 0)
    {
        ui->lineEditCustomParsingRules->setEnabled(false);
        ui->labelParsingRules->setEnabled(false);
        ui->comboBoxGraphDisplayMode->setPalette(QPalette()); // Empty = default (OS specific !)
    }
    else
    {
        ui->lineEditCustomParsingRules->setEnabled(true);
        ui->labelParsingRules->setEnabled(true);
        ui->comboBoxGraphDisplayMode->setPalette(*paletteRed);
    }
}

void MainWindow::on_pushButtonClearLogTable_clicked()
{
    ui->tableWidgetLogTable->setRowCount(0);
    ui->tableWidgetLogTable->setColumnCount(1);
}

void MainWindow::on_checkBoxScrollLogEnableSorting_toggled(bool checked)
{
    ui->tableWidgetLogTable->setSortingEnabled(checked);
}

// https://stackoverflow.com/questions/27353026/qtableview-output-save-as-csv-or-txt
void MainWindow::exportTableLogToCSV(QTableView *table, QChar sep)
{
    QString filters("CSV files (*.csv);;All files (*.*)");
    QString defaultFilter("CSV files (*.csv)");
    QString fileName = QFileDialog::getSaveFileName(0, "Save file", QCoreApplication::applicationDirPath(),
                                                    filters, &defaultFilter);
    QFile file(fileName);

    QAbstractItemModel *model = table->model();

    if (file.open(QFile::WriteOnly | QFile::Truncate))
    {
        QTextStream data(&file);
        QStringList strList;
        for (int i = 0; i < model->columnCount(); ++i)
        {
            if (model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString().length() > 0)
                strList.append("\"" + model->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() + "\"");
            else
                strList.append("");
        }

        data << strList.join(sep) << "\n";

        for (int i = 0; i < model->rowCount(); ++i)
        {
            strList.clear();
            for (int j = 0; j < model->columnCount(); ++j)
            {
                if (model->data(model->index(i, j)).toString().length() > 0)
                    strList.append("\"" + model->data(model->index(i, j)).toString() + "\"");
                else
                    strList.append("");
            }
            data << strList.join(sep) + "\n";
        }
        file.close();
    }
}

void MainWindow::exportArraysToCSV(QStringList labelList, QList<QList<double>> dataColums, QChar sep)
{
    QString filters("CSV files (*.csv);;All files (*.*)");
    QString defaultFilter("CSV files (*.csv)");
    QString fileName = QFileDialog::getSaveFileName(0, "Save file", QCoreApplication::applicationDirPath(),
                                                    filters, &defaultFilter);
    QFile file(fileName);

    if (file.open(QFile::WriteOnly | QFile::Truncate))
    {
        QTextStream data(&file);
        QStringList strList;

        foreach (auto label, labelList)
        {
            if (label.length() > 0)
                strList.append("\"" + label + "\"");
            else
                strList.append("");
        }

        data << strList.join(sep) << "\n";

        int maxRowCount = 0;
        foreach (auto column, dataColums)
            maxRowCount = qMax(maxRowCount, column.count());

        for (int i = 0; i < maxRowCount; ++i) // rows
        {
            strList.clear();
            for (int j = 0; j < dataColums.count(); ++j) // columns
            {
                if (i < dataColums[j].count())
                    strList.append(QString::number(dataColums[j][i], 'f'));
                else
                    strList.append("\"\"");
            }
            data << strList.join(sep) + "\n";
        }
        file.close();
    }
}

void MainWindow::on_pushButtonExportLogTableToCSV_clicked()
{
    this->exportTableLogToCSV(ui->tableWidgetLogTable);
}

void MainWindow::on_pushButtonSerialLogScrollown_clicked()
{
    ui->textBrowserLogs->verticalScrollBar()->setValue(ui->textBrowserLogs->verticalScrollBar()->maximum());
}

void MainWindow::on_pushButtonEnableTableLog_toggled(bool checked)
{
    if (checked)
    {
        ui->pushButtonEnableTableLog->setChecked(true);
        ui->pushButtonEnableTableLog->setText("Enable");
    }
    else
    {
        ui->pushButtonEnableTableLog->setChecked(false);
        ui->pushButtonEnableTableLog->setText("Disable");
    }
}

void MainWindow::on_lineEditLoadFilePath_textChanged(const QString &arg1)
{
    QFile file(ui->lineEditLoadFilePath->text());

    if (!file.exists())
    {
        ui->lineEditFileInfo->setText("File doesnt exist !");
        return;
    }

    QList<QTime> timeRange = fileReader.getFileTimeRange(&file);
    ui->timeEditMinParsingTime->setTime(timeRange.first());
    ui->timeEditMaxParsingTime->setTime(timeRange.last());

    QString info = "Size: " + QString::number(file.size());
    ui->lineEditFileInfo->setText(info);
}

void MainWindow::on_actionFull_parser_data_triggered()
{
    ui->splitterGraphTable->setSizes({0, ui->splitterGraphTable->height()});
}

void MainWindow::on_actionImage_triggered()
{
    QString default_name = "export.png";
    QString filename = QFileDialog::getSaveFileName(this, tr("Save File"), QDir::homePath() + "/" + default_name, tr("Image Files (*.png *.jpg *.bmp)"));

    if (filename.isEmpty())
        return;

    QImage bitmap(ui->widgetChart->size(), QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&bitmap);
    ui->widgetChart->render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
    QImageWriter writer(filename, "png");
    writer.write(bitmap);
    qDebug() << "Wrote image to file";
}

//void MainWindow::on_actionto_csv_triggered()
//{

//    //    QStringList labelList = parser.getLabelStorage();

//    //    QStringList columnNames = labelList;
//    //    QList<QList<double>> columnsData;

//    //    QList<double> numericDataList = parser.getDataStorage();

//    //    columnNames.removeDuplicates();

//    //    for (auto i = 0; i < columnNames.count(); ++i)
//    //    {
//    //        columnsData.append(*new QList<double>);

//    //        while (labelList.contains(columnNames[i]))
//    //        {
//    //            columnsData[columnsData.count() - 1].append(numericDataList.takeAt(labelList.indexOf(columnNames[i])));
//    //            labelList.removeAt(labelList.indexOf(columnNames[i]));
//    //        }
//    //    }

//}

void MainWindow::on_actionto_csv_triggered()
{
    QStringList columnNames;
    QList<QList<double>> columnsData;

    parser.getCSVReadyData(&columnNames, &columnsData);

    this->exportArraysToCSV(columnNames, columnsData, ',');
}
