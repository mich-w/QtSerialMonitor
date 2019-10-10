#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QHostInfo>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    createTimers();
    createChart();
    setupTable();
    setupGUI();
    //  create3DView();

    settingsLoadAll();

    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuitSlot()));
}

MainWindow::~MainWindow()
{
    delete ui;
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

void MainWindow::create3DView()
{
    //  QSurfaceFormat::setDefaultFormat(Q3DS::surfaceFormat());
    // ui->widget3DView->presentation()->setProfilingEnabled(true);
    //  ui->widget3DView->presentation()->setSource(QUrl("qrc:/presentation2/SampleProject.uia"));

    //    QObject *object = ui->widget3DView->presentation();
    //    QObject *rect = object->findChild<QObject*>("Body");
    //   if (rect)
    //   {
    //        qDebug() << "Found rect ";
    //        rect->setProperty("width", 500);
    //   }
}

void MainWindow::printChangeLog() // TODO
{
    //    ui->textBrowserLogs->append(CHANGELOG_TEXT);
    //    ui->textBrowserLogs->horizontalScrollBar()->setValue(0);
}

void MainWindow::settingsLoadAll()
{
    QSettings appSettings(QSettings::Format::IniFormat,
                          QSettings::Scope::SystemScope,
                          "QtSerialMonitor", "QtSerialMonitor"); // Settings

    // ----- Info ----- //
    {
        if (appSettings.value("Info/version").value<QString>() != VERSION)
        {
            printChangeLog();
        }

        if (appSettings.value("Info/organizationName").value<QString>() != appSettings.organizationName() &&
                appSettings.value("Info/applicationName").value<QString>() != appSettings.applicationName())
        {
            qDebug() << "Abort loading settings ! organizationName or applicationName incorrect. Config file might be missing.";
            return;
        }
    }
    // ------------------------- //

    // ----- splitterSizes ----- //
    {
        QStringList splitterSizes = appSettings.value("layout/splitterSizes").value<QString>().split(QRegExp("\\s+"), QString::SplitBehavior::SkipEmptyParts);
        if (!splitterSizes.isEmpty())
            ui->splitterReceivedData->setSizes(QList<int>({splitterSizes.first().trimmed().toInt(), splitterSizes.last().trimmed().toInt()}));
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
        ui->spinBoxProcessingDelay->setValue(appSettings.value("layout/spinBoxProcessingDelay.value").value<int>());
        ui->spinBoxMaxLines->setValue(appSettings.value("layout/spinBoxMaxLines.value").value<int>());
        ui->spinBoxMaxGraphs->setValue(appSettings.value("layout/spinBoxMaxGraphs.value").value<int>());
        ui->spinBoxMaxTimeRange->setValue(appSettings.value("layout/spinBoxMaxTimeRange.value").value<int>());
        ui->spinBoxScrollingTimeRange->setValue(appSettings.value("layout/spinBoxScrollingTimeRange.value").value<int>());
        ui->spinBoxRemoveOldLabels->setValue(appSettings.value("layout/spinBoxRemoveOldLabels.value").value<int>());
        ui->spinBoxUDPTargetPort->setValue(appSettings.value("layout/spinBoxUDPTargetPort.value").value<int>());
        ui->spinBoxUDPReceivePort->setValue(appSettings.value("layout/spinBoxUDPReceivePort.value").value<int>());

        ui->comboBoxParity->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxParity.currentIndex").value<int>());
        ui->comboBoxDataBits->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxDataBits.currentIndex").value<int>());
        ui->comboBoxStopBits->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxStopBits.currentIndex").value<int>());
        ui->comboBoxBaudRates->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxBaudRates.currentIndex").value<int>());
        ui->comboBoxFlowControl->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxFlowControl.currentIndex").value<int>());
        ui->comboBoxGraphDisplayMode->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxGraphDisplayMode.currentIndex").value<int>());
        ui->comboBoxTracerStyle->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxTracerStyle.currentIndex").value<int>());
        ui->comboBoxSerialReadMode->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxSerialReadMode.currentIndex").value<int>());
        ui->comboBoxUDPSendMode->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxUDPSendMode.currentIndex").value<int>());
        ui->comboBoxUDPReceiveMode->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxUDPReceiveMode.currentIndex").value<int>());
        ui->comboBoxExternalTimeFormat->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxExternalTimeFormat.currentIndex").value<int>());
        ui->comboBoxRAMLoadMode->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxRAMLoadMode.currentIndex").value<int>());
        ui->comboBoxRAMSaveMode->setCurrentIndex(appSettings.value("GUI_Elements/comboBoxRAMSaveMode.currentIndex").value<int>());

        ui->checkBoxDTR->setChecked(appSettings.value("GUI_Elements/checkBoxDTR.isChecked").value<bool>());
        ui->checkBoxTrim->setChecked(appSettings.value("GUI_Elements/checkBoxTrim.isChecked").value<bool>());
        ui->checkBoxSendKey->setChecked(appSettings.value("GUI_Elements/checkBoxSendKey.isChecked").value<bool>());
        ui->checkBoxWrapText->setChecked(appSettings.value("GUI_Elements/checkBoxWrapText.isChecked").value<bool>());
        ui->checkBoxAutoTrack->setChecked(appSettings.value("GUI_Elements/checkBoxAutoTrack.isChecked").value<bool>());
        ui->checkBoxAutoRefresh->setChecked(appSettings.value("GUI_Elements/checkBoxAutoRefresh.isChecked").value<bool>());
        ui->checkBoxAutoRescaleY->setChecked(appSettings.value("GUI_Elements/checkBoxAutoRescaleY.isChecked").value<bool>());
        ui->checkBoxEnableTracer->setChecked(appSettings.value("GUI_Elements/checkBoxEnableTracer.isChecked").value<bool>());
        ui->checkBoxAutoLogging->setChecked(appSettings.value("GUI_Elements/checkBoxAutoLogging.isChecked").value<bool>());
        ui->checkBoxAppendDate->setChecked(appSettings.value("GUI_Elements/checkBoxAppendDate.isChecked").value<bool>());
        ui->checkBoxSimplifyLog->setChecked(appSettings.value("GUI_Elements/checkBoxSimplifyLog.isChecked").value<bool>());
        ui->checkBoxRAMClearChart->setChecked(appSettings.value("GUI_Elements/checkBoxRAMClearChart.isChecked").value<bool>());
        ui->checkBoxEnableRAMBuffer->setChecked(appSettings.value("GUI_Elements/checkBoxEnableRAMBuffer.isChecked", true).value<bool>()); // default value set to true !

        ui->pushButtonEnablePlot->setChecked(appSettings.value("GUI_Elements/pushButtonEnablePlot.isChecked").value<bool>());

        ui->tabWidgetConnectivity->setCurrentIndex(appSettings.value("GUI_Elements/tabWidgetConnectivity.currentIndex").value<int>());

        ui->lineEditCustomParsingRules->setText(appSettings.value("data/lineEditCustomParsingRules.text").value<QString>());
        ui->lineEditUDPTargetIP->setText(appSettings.value("data/lineEditUDPTargetIP.text").value<QString>());
        ui->lineEditSaveLogPath->setText(appSettings.value("data/lineEditSaveLogPath.text").value<QString>());
        ui->lineEditLoadFilePath->setText(appSettings.value("data/lineEditLoadFilePath.text").value<QString>());
        ui->lineEditSaveFileName->setText(appSettings.value("data/lineEditSaveFileName.text").value<QString>());
        ui->lineEditExternalClockLabel->setText(appSettings.value("data/lineEditExternalClockLabel.text").value<QString>());

        if (ui->lineEditLoadFilePath->text().isEmpty() == false)
        {
            QFile file(ui->lineEditLoadFilePath->text());
            getFileTimeRange(&file);
        }

        qDebug() << "Loaded settings";
    }
}

void MainWindow::settingsSaveAll()
{
    QSettings appSettings(QSettings::Format::IniFormat,
                          QSettings::Scope::SystemScope,
                          "QtSerialMonitor", "QtSerialMonitor"); // Settings

    // ----- Info ----- //
    {
        appSettings.setValue("Info/version", VERSION);
        appSettings.setValue("Info/organizationName", appSettings.organizationName());
        appSettings.setValue("Info/applicationName", appSettings.applicationName());
    }
    // ------------------------- //

    // ----- splitterSizes ----- //
    {
        QString splitterSizes = QString::number(ui->splitterReceivedData->sizes().first()) + " " + QString::number(ui->splitterReceivedData->sizes().last());
        appSettings.setValue("layout/splitterSizes", splitterSizes);
    }
    // ------------------------- //

    // ----- GUI Elements ----- //
    {
        appSettings.setValue("layout/spinBoxProcessingDelay.value", ui->spinBoxProcessingDelay->value());
        appSettings.setValue("layout/spinBoxMaxLines.value", ui->spinBoxMaxLines->value());
        appSettings.setValue("layout/spinBoxMaxGraphs.value", ui->spinBoxMaxGraphs->value());
        appSettings.setValue("layout/spinBoxMaxTimeRange.value", ui->spinBoxMaxTimeRange->value());
        appSettings.setValue("layout/spinBoxScrollingTimeRange.value", ui->spinBoxScrollingTimeRange->value());
        appSettings.setValue("layout/spinBoxRemoveOldLabels.value", ui->spinBoxRemoveOldLabels->value());
        appSettings.setValue("layout/spinBoxUDPTargetPort.value", ui->spinBoxUDPTargetPort->value());
        appSettings.setValue("layout/spinBoxUDPReceivePort.value", ui->spinBoxUDPReceivePort->value());

        appSettings.setValue("GUI_Elements/comboBoxParity.currentIndex", ui->comboBoxParity->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxDataBits.currentIndex", ui->comboBoxDataBits->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxStopBits.currentIndex", ui->comboBoxStopBits->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxBaudRates.currentIndex", ui->comboBoxBaudRates->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxFlowControl.currentIndex", ui->comboBoxFlowControl->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxGraphDisplayMode.currentIndex", ui->comboBoxGraphDisplayMode->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxTracerStyle.currentIndex", ui->comboBoxTracerStyle->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxSerialReadMode.currentIndex", ui->comboBoxSerialReadMode->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxUDPSendMode.currentIndex", ui->comboBoxUDPSendMode->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxUDPReceiveMode.currentIndex", ui->comboBoxUDPReceiveMode->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxExternalTimeFormat.currentIndex", ui->comboBoxExternalTimeFormat->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxRAMLoadMode.currentIndex", ui->comboBoxRAMLoadMode->currentIndex());
        appSettings.setValue("GUI_Elements/comboBoxRAMSaveMode.currentIndex", ui->comboBoxRAMSaveMode->currentIndex());

        appSettings.setValue("GUI_Elements/checkBoxDTR.isChecked", ui->checkBoxDTR->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxTrim.isChecked", ui->checkBoxTrim->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxSendKey.isChecked", ui->checkBoxSendKey->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxWrapText.isChecked", ui->checkBoxWrapText->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxAutoTrack.isChecked", ui->checkBoxAutoTrack->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxAutoRefresh.isChecked", ui->checkBoxAutoRefresh->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxAutoRescaleY.isChecked", ui->checkBoxAutoRescaleY->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxEnableTracer.isChecked", ui->checkBoxEnableTracer->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxAutoLogging.isChecked", ui->checkBoxAutoLogging->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxAppendDate.isChecked", ui->checkBoxAppendDate->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxSimplifyLog.isChecked", ui->checkBoxSimplifyLog->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxRAMClearChart.isChecked", ui->checkBoxRAMClearChart->isChecked());
        appSettings.setValue("GUI_Elements/checkBoxEnableRAMBuffer.isChecked", ui->checkBoxEnableRAMBuffer->isChecked());

        appSettings.setValue("GUI_Elements/pushButtonEnablePlot.isChecked", ui->pushButtonEnablePlot->isChecked());

        appSettings.setValue("GUI_Elements/tabWidgetConnectivity.currentIndex", ui->tabWidgetConnectivity->currentIndex());

        appSettings.setValue("data/lineEditCustomParsingRules.text", ui->lineEditCustomParsingRules->text());
        appSettings.setValue("data/lineEditUDPTargetIP.text", ui->lineEditUDPTargetIP->text());
        appSettings.setValue("data/lineEditSaveLogPath.text", ui->lineEditSaveLogPath->text());
        appSettings.setValue("data/lineEditLoadFilePath.text", ui->lineEditLoadFilePath->text());
        appSettings.setValue("data/lineEditSaveFileName.text", ui->lineEditSaveFileName->text());
        appSettings.setValue("data/lineEditExternalClockLabel.text", ui->lineEditExternalClockLabel->text());
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

void MainWindow::aboutToQuitSlot()
{
    settingsSaveAll();
}

void MainWindow::setupGUI()
{
    qDebug() << QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss:zzz");

    this->setWindowTitle(this->windowTitle() + " " + VERSION);

    ui->textBrowserLogs->document()->setMaximumBlockCount(ui->spinBoxMaxLines->value());

    foreach (auto item, QSerialPortInfo::standardBaudRates())
        ui->comboBoxBaudRates->addItem(QString::number(item));

    ui->comboBoxBaudRates->setCurrentIndex(ui->comboBoxBaudRates->count() - 3); // TODO SETTINGS !

    ui->comboBoxTracerStyle->addItem("Crosshair");
    ui->comboBoxTracerStyle->addItem("Circle");
    ui->comboBoxTracerStyle->setCurrentIndex(0);

    connect(ui->comboBoxSend->lineEdit(), SIGNAL(returnPressed()), this, SLOT(comboBoxSendReturnPressedSlot()));

    ui->comboBoxGraphDisplayMode->addItem("Auto");
    ui->comboBoxGraphDisplayMode->addItem("Custom");
    ui->comboBoxGraphDisplayMode->setCurrentIndex(0);

    ui->comboBoxDataBits->addItem("Data5");
    ui->comboBoxDataBits->addItem("Data6");
    ui->comboBoxDataBits->addItem("Data7");
    ui->comboBoxDataBits->addItem("Data8");
    ui->comboBoxDataBits->setCurrentIndex(3);

    ui->comboBoxParity->addItem("NoParity");
    ui->comboBoxParity->addItem("EvenParity");
    ui->comboBoxParity->addItem("OddParity");
    ui->comboBoxParity->addItem("SpaceParity");
    ui->comboBoxParity->addItem("MarkParity");
    ui->comboBoxParity->setCurrentIndex(0);

    ui->comboBoxStopBits->addItem("OneStop");
    ui->comboBoxStopBits->addItem("OneAndHalfStop");
    ui->comboBoxStopBits->addItem("TwoStop");
    ui->comboBoxStopBits->setCurrentIndex(0);

    ui->comboBoxFlowControl->addItem("NoFlowControl");
    ui->comboBoxFlowControl->addItem("HardwareControl");
    ui->comboBoxFlowControl->addItem("SoftwareControl");
    ui->comboBoxFlowControl->setCurrentIndex(0);

    ui->comboBoxSerialReadMode->addItem("canReadLine | readLine");
    ui->comboBoxSerialReadMode->addItem("canReadLine | readAll");
    ui->comboBoxSerialReadMode->addItem("bytesAvailable | readLine");
    ui->comboBoxSerialReadMode->addItem("bytesAvailable | readAll");
    ui->comboBoxSerialReadMode->setCurrentIndex(0);

    updateDeviceList();

    emit on_checkBoxShowLegend_toggled(ui->checkBoxShowLegend->isChecked());

    ui->comboBoxUDPReceiveMode->addItem("Any");
    ui->comboBoxUDPReceiveMode->addItem("LocalHost");
    ui->comboBoxUDPReceiveMode->addItem("SpecialAddress");
    ui->comboBoxUDPReceiveMode->setCurrentIndex(0);

    ui->comboBoxUDPSendMode->addItem("Broadcast");
    ui->comboBoxUDPSendMode->addItem("LocalHost");
    ui->comboBoxUDPSendMode->addItem("SpecialAddress");
    ui->comboBoxUDPSendMode->setCurrentIndex(0);

    // ui->lineEditUDPTargetIP->setInputMask( "000.000.000.000" );

    if (ui->checkBoxAutoRefresh->isChecked())
    {
        serialDeviceCheckTimer->start(500);
        ui->pushButtonRefresh->setEnabled(false);
    }
    else
    {
        ui->pushButtonRefresh->setEnabled(true);
    }

    ui->lineEditSaveLogPath->setText(qApp->applicationDirPath() + "/Logs");
    ui->lineEditSaveFileName->setText("Log.txt");

    ui->splitterGraphTable->setSizes({this->height(), 0});

    ui->comboBoxExternalTimeFormat->addItem("[ms]");
    ui->comboBoxExternalTimeFormat->setCurrentIndex(0);

    emit on_checkBoxExternalTimeReference_toggled(ui->checkBoxExternalTimeReference->isChecked());

    ui->comboBoxLoggingMode->addItem("Log Text");
    ui->comboBoxLoggingMode->addItem("Log Parsed Data");
    ui->comboBoxLoggingMode->setCurrentIndex(0);

    ui->comboBoxRAMSaveMode->addItem("Save Data Only");
    ui->comboBoxRAMSaveMode->addItem("Save Data & Text");
    ui->comboBoxRAMSaveMode->setCurrentIndex(0);

    ui->comboBoxRAMLoadMode->addItem("Load Data Only");
    ui->comboBoxRAMLoadMode->addItem("Load Data & Text");
    ui->comboBoxRAMLoadMode->setCurrentIndex(0);

    ui->lineEditLoadFilePath->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
}

void MainWindow::comboBoxSendReturnPressedSlot()
{
    if (ui->pushButtonSerialConnect->isChecked())
        sendSerial(ui->comboBoxSend->currentText());
    if (ui->pushButtonUDPConnect->isChecked())
        sendDatagram(ui->comboBoxSend->currentText());

    ui->comboBoxSend->setCurrentText("");
    ui->comboBoxSend->model()->sort(0, Qt::SortOrder::AscendingOrder); // sort alphabetically
}

void MainWindow::createTimers()
{
    this->serialDeviceCheckTimer = new QTimer(this);
    this->radioButtonTimer = new QTimer(this);
    this->serialStringProcessingTimer = new QTimer(this);
    this->udpStringProcessingTimer = new QTimer(this);

    connect(serialDeviceCheckTimer, SIGNAL(timeout()), this, SLOT(updateDeviceList()));
    connect(radioButtonTimer, &QTimer::timeout, this, [=]() { ui->radioButtonDeviceUpdate->setChecked(false); });
}

//void MainWindow::getHostList()
//{
//    ui->comboBoxTargetIP->clear();

//    foreach (QNetworkInterface netInterface, QNetworkInterface::allInterfaces())
//    {
//        foreach (QNetworkAddressEntry address, netInterface.addressEntries())
//        {
//            if(address.ip().protocol() == QAbstractSocket::IPv4Protocol)
//                ui->comboBoxTargetIP->addItem(QHostAddress(address.ip()).toString());
//        }
//    }

//    foreach (const QHostAddress& address, QHostInfo::fromName(QHostInfo::localHostName()).addresses())
//    {
//        if (address.protocol() == QAbstractSocket::IPv4Protocol && address.isLoopback() == false)
//        {
//            //   ui->lineEditLocalIP->setText(QHostAddress(QHostAddress::LocalHost).toString());
//        }
//    }
//}

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

    connect(ui->widgetChart, SIGNAL(mouseDoubleClick(QMouseEvent *)), this, SLOT(chartMouseDoubleClickHandler(QMouseEvent *)));
    connect(ui->widgetChart, SIGNAL(mousePress(QMouseEvent *)), this, SLOT(chartMousePressHandler(QMouseEvent *)));
    connect(ui->widgetChart, SIGNAL(selectionChangedByUser()), this, SLOT(chartSelectionChanged()));
    connect(ui->widgetChart, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(chartContextMenuRequest(QPoint)));
    connect(ui->widgetChart, SIGNAL(beforeReplot()), this, SLOT(chartBeforeReplotSlot()));

    //  ui->widgetChart->setNotAntialiasedElements(QCP::aeAll);

    if (ui->checkBoxEnableTracer->isChecked())
    {
        createChartTracer();
        connect(ui->widgetChart, SIGNAL(mouseMove(QMouseEvent *)), this, SLOT(showPointValue(QMouseEvent *)));
    }
}

// Auto-scroll X and rescale Y
void MainWindow::chartBeforeReplotSlot()
{
    if (ui->checkBoxAutoTrack->isChecked() && ui->widgetChart->graphCount() > 0)
    {
        if (parser.getListTimeStamp().count() > 0)
        {
            ui->widgetChart->xAxis->setRange((parser.getListTimeStamp().last() / 1000.0) + (ui->spinBoxScrollingTimeRange->value() * 0.05),
                                             ui->spinBoxScrollingTimeRange->value(), Qt::AlignRight);
        }

        if (ui->checkBoxAutoRescaleY->isChecked())
        {
            bool atLeastOneGraphVisible = false, atLeastOneNotFlat = false;

            for (auto i = 0; i < ui->widgetChart->graphCount(); ++i)
            {
                if (ui->widgetChart->graph(i)->visible())
                {
                    atLeastOneGraphVisible = true;

                    bool foundRange = false;
                    QCPRange valueRange = ui->widgetChart->graph(i)->data().data()->valueRange(foundRange);

                    if (foundRange && abs(valueRange.upper - valueRange.lower) >= 0.1)
                        atLeastOneNotFlat = true;
                }
            }

            if (atLeastOneGraphVisible && atLeastOneNotFlat)
            {
                ui->widgetChart->yAxis->rescale(true);
                ui->widgetChart->yAxis->scaleRange(1.20);
            }
        }
    }
    else
    {
        //        if (ui->widgetChart->graph()->visible() == false)
        //            ui->widgetChart->rescaleAxes();
    }
}

void MainWindow::chartContextMenuRequest(QPoint pos)
{
    QMenu *menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    if (ui->widgetChart->legend->selectTest(pos, false) >= 0) // context menu on legend requested
    {
        clearGraphSelection();

        for (auto i = 0; i < ui->widgetChart->legend->itemCount(); ++i)
        {
            if (ui->widgetChart->legend->item(i)->selectTest(pos, false) >= 0)
            {
                ui->widgetChart->legend->item(i)->setSelected(true);
                menu->addAction("Show excusively", this, SLOT(showSelectedGraphExclusively()));

                if (ui->widgetChart->graph(i)->visible())
                    menu->addAction("Hide graph", this, SLOT(hideSelectedGraph()));
                else
                    menu->addAction("Show graph", this, SLOT(showSelectedGraph()));
            }
        }
    }
    else
    {
        for (auto i = 0; i < ui->widgetChart->legend->itemCount(); ++i) // context menu requested eslewhere on the graph
        {
            if (ui->widgetChart->legend->item(i)->selected())
            {
                menu->addAction("Show excusively", this, SLOT(showSelectedGraphExclusively()));

                if (ui->widgetChart->graph(i)->visible())
                    menu->addAction("Hide graph", this, SLOT(hideSelectedGraph()));
                else
                    menu->addAction("Show graph", this, SLOT(showSelectedGraph()));
            }
        }
    }

    if (ui->widgetChart->graphCount() > 0)
    {
        menu->addAction("Show all graphs", this, SLOT(showAllGraphs()));
        menu->addAction("Hide all graphs", this, SLOT(hideAllGraphs()));
    }

    menu->popup(ui->widgetChart->mapToGlobal(pos));
    ui->widgetChart->replot();
}

void MainWindow::showSelectedGraphExclusively()
{
    hideAllGraphs();
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

void MainWindow::showAllGraphs()
{
    for (auto i = 0; i < ui->widgetChart->legend->itemCount(); ++i)
    {
        ui->widgetChart->graph(i)->setVisible(true);
        ui->widgetChart->legend->item(i)->setTextColor(Qt::GlobalColor::black);
    }

    ui->widgetChart->replot();
}

void MainWindow::clearGraphSelection()
{
    for (auto i = 0; i < ui->widgetChart->graphCount(); ++i)
    {
        ui->widgetChart->legend->item(i)->setSelected(false);
    }
}

void MainWindow::hideSelectedGraph()
{
    for (auto i = 0; i < ui->widgetChart->legend->itemCount(); ++i)
    {
        if (ui->widgetChart->legend->item(i)->selected())
        {
            ui->widgetChart->graph(i)->setVisible(false);
            ui->widgetChart->legend->item(i)->setTextColor(Qt::GlobalColor::gray);
        }
    }

    clearGraphSelection();
    ui->widgetChart->replot();
}

void MainWindow::showSelectedGraph()
{
    for (auto i = 0; i < ui->widgetChart->legend->itemCount(); ++i)
    {
        if (ui->widgetChart->legend->item(i)->selected())
        {
            ui->widgetChart->graph(i)->setVisible(true);
            ui->widgetChart->legend->item(i)->setTextColor(Qt::GlobalColor::black);
        }
    }

    clearGraphSelection();
    ui->widgetChart->replot();
}

void MainWindow::hideAllGraphs()
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

void MainWindow::chartMouseDoubleClickHandler(QMouseEvent *event)
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

void MainWindow::chartSelectionChanged()
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

void MainWindow::showPointValue(QMouseEvent *event)
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

void MainWindow::chartMousePressHandler(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton)
        ui->widgetChart->setSelectionRectMode(QCP::srmZoom);
    else
        ui->widgetChart->setSelectionRectMode(QCP::srmNone);

    ui->widgetChart->replot();
}

void MainWindow::updateDeviceList()
{
    QList<QSerialPortInfo> devices;
    QList<QString> portNames;
    static QList<QString> portNamesOld;

    devices = QSerialPortInfo::availablePorts();

    foreach (auto item, devices)
    {
        portNames.append(item.portName());
    }

    if ((devices.count() >= 1) && !(portNames.toSet().intersects(portNamesOld.toSet())))
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
        ui->comboBoxDevices->addItem("No COM devices detected :(");
        ui->comboBoxDevices->setCurrentIndex(ui->comboBoxDevices->count() - 1);
    }

    portNamesOld = portNames;

    this->radioButtonTimer->start(100);
    ui->radioButtonDeviceUpdate->setChecked(true);
}

void MainWindow::on_pushButtonRefresh_clicked()
{
    qDebug() << "Szukam urządzeń COM...";
    this->addLog(">>\t Searching for COM ports...");
    this->updateDeviceList();
}

void MainWindow::addLog(QString text)
{
    if (ui->pushButtonTextLogToggle->isChecked() == false)
    {
        QString currentDateTime = QDateTime::currentDateTime().toString("hh:mm:ss:zzz ");

        if (ui->checkBoxShowTime->isChecked())
            text = currentDateTime + text;

        ui->textBrowserLogs->append(text);

        // ui->textBrowserLogs->insertPlainText(text);
        // ui->textBrowserLogs->moveCursor(QTextCursor::MoveOperation::End, QTextCursor::MoveMode::MoveAnchor);
        // ui->textBrowserLogs->verticalScrollBar()->setValue(ui->textBrowserLogs->verticalScrollBar()->maximum());
    }
}

void MainWindow::processSerial()
{
    QString serialInput = serial.getString();

    if (serialInput.isEmpty() == false)
    {
        if (ui->checkBoxTrim->isChecked()) // Append text to textBrowser
            addLog("Serial <<\t" + serialInput.trimmed());
        else
            addLog("Serial <<\t" + serialInput);

        parser.parse(serialInput, ui->checkBoxSyncSystemClock->isChecked(), ui->checkBoxExternalTimeReference->isChecked(), ui->lineEditExternalClockLabel->text()); // Parse string - split into labels + numeric data
        QStringList labelList = parser.getStringListLabels();
        QList<double> numericDataList = parser.getListNumericValues();
        QList<long> timeStamps = parser.getListTimeStamp();

        this->writeLogToFile(serialInput, labelList, numericDataList, timeStamps);
        this->processChart(labelList, numericDataList, timeStamps);
        this->saveToRAM(labelList, numericDataList, timeStamps, ui->comboBoxRAMSaveMode->currentIndex(), serialInput);
        this->processTable(labelList, numericDataList); // Fill tableWidget
    }
}

void MainWindow::writeLogToFile(QString rawLine, QStringList labelList, QList<double> dataList, QList<long> timeList)
{
    if (ui->pushButtonLogging->isChecked()) // Write log into file
    {
        if (ui->comboBoxLoggingMode->currentIndex() == 0)
            fileLogger.writeLogLine(rawLine, ui->checkBoxSimplifyLog->isChecked(), ui->checkBoxAppendDate->isChecked());
        else if (ui->comboBoxLoggingMode->currentIndex() == 1)
            fileLogger.writeLogParsedData(labelList, dataList, ui->checkBoxAppendDate->isChecked());
    }
}

void MainWindow::processUDP()
{
    QString udpInput = networkUDP.readString();
    if (udpInput.isEmpty() == false)
    {
        if (ui->checkBoxTrim->isChecked())
            addLog("UDP <<\t" + udpInput.trimmed());
        else
            addLog("UDP <<\t" + udpInput);

        parser.parse(udpInput, ui->checkBoxSyncSystemClock->isChecked(), ui->checkBoxExternalTimeReference->isChecked(), ui->lineEditExternalClockLabel->text()); // Parse string - split into labels + numeric data
        QStringList labelList = parser.getStringListLabels();
        QList<double> numericDataList = parser.getListNumericValues();
        QList<long> timeStamps = parser.getListTimeStamp();

        this->writeLogToFile(udpInput, labelList, numericDataList, timeStamps);
        this->processChart(labelList, numericDataList, timeStamps);
        this->saveToRAM(labelList, numericDataList, timeStamps, ui->comboBoxRAMSaveMode->currentIndex(), udpInput);
        this->processTable(labelList, numericDataList); // Fill tableWidget
    }
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
                  ui->lineEditCustomParsingRules->text().simplified().contains(label, Qt::CaseSensitivity::CaseSensitive))))
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
                if (timeStampsList[j] > 0) // if == 0 then we assume taht no external clock was parsed propely
                    ui->widgetChart->graph(i)->addData(timeStampsList[j] / 1000.0, numericDataList[j]);

                // break; // we keep checking for duplicates so we can add multiple points at once
            }
        }

        if (ui->spinBoxMaxTimeRange->value() > 0)
            ui->widgetChart->graph(i)->data().data()->removeBefore((timeStampsList.last() / 1000.0) - ui->spinBoxMaxTimeRange->value()); // Remove old samples

        if (ui->spinBoxRemoveOldLabels->value() > 0)
        {
            if (labelList.contains(ui->widgetChart->graph(i)->name()))
                missingCount[i] = 0;
            else
                missingCount[i]++;

            if (missingCount[i] > ui->spinBoxRemoveOldLabels->value())
            {
                ui->widgetChart->removeGraph(i);
                missingCount.removeAt(i);
            }
        }
    }

    ui->widgetChart->replot();
}

void MainWindow::setSelectedLabels(QList<QString> *labels, bool customRules)
{
    qDebug() << "setSelectedLabels";

    if (!customRules)
    {
        if (labels->count() < 1)
        {
            showAllGraphs();
            ui->lineEditCustomParsingRules->clear();
        }
        else
        {
            hideAllGraphs();

            if (ui->widgetChart->graphCount() > 0)
            {
                for (auto i = 0; i < ui->widgetChart->graphCount(); ++i)
                {
                    if (labels->contains(ui->widgetChart->graph(i)->name()))
                    {
                        ui->widgetChart->legend->item(i)->setSelected(true);
                        showSelectedGraph();
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
            if (ui->pushButtonSerialConnect->isChecked())
                sendSerial(event->text());
            if (ui->pushButtonUDPConnect->isChecked())
                sendDatagram(event->text());
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

    if (event->key() == Qt::Key_F1)
    {
        QWhatsThis::enterWhatsThisMode();
    }
}

void MainWindow::sendSerial(QString message)
{
    if (serial.send(message))
        this->addLog("S >>\t" + message);
    else
        this->addLog("S >>\t Unable to send! Serial port closed !");
}

void MainWindow::on_checkBoxAutoRefresh_toggled(bool checked)
{
    if (checked == true)
    {
        ui->pushButtonRefresh->setEnabled(false);
        serialDeviceCheckTimer->start(500);
    }
    else
    {
        ui->pushButtonRefresh->setEnabled(true);
        serialDeviceCheckTimer->stop();
    }
}

void MainWindow::on_pushButtonScrollToButtom_clicked()
{
    ui->textBrowserLogs->verticalScrollBar()->setValue(ui->textBrowserLogs->verticalScrollBar()->maximum());
}

void MainWindow::on_pushButtonClear_clicked()
{
    ui->textBrowserLogs->clear();
}

void MainWindow::on_lineEditHighlight_returnPressed()
{
    highlighLog(ui->lineEditHighlight->text());
}

void MainWindow::highlighLog(QString searchString)
{
    QTextDocument *document = ui->textBrowserLogs->document();
    QTextCursor highlightCursor(document);
    QTextCharFormat colorFormat = highlightCursor.charFormat();

    // clear formatting !
    colorFormat.setFontLetterSpacing(100);
    colorFormat.setForeground(Qt::black);
    colorFormat.setFontWeight(QFont::Thin);
    colorFormat.setBackground(Qt::white);
    highlightCursor.setPosition(document->characterCount() - 1, QTextCursor::KeepAnchor);
    highlightCursor.mergeCharFormat(colorFormat);
    highlightCursor.setPosition(0, QTextCursor::MoveAnchor);

    if (!searchString.isEmpty())
    {
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
        ui->textBrowserLogs->setLineWrapMode(QTextBrowser::LineWrapMode::WidgetWidth);
    else
        ui->textBrowserLogs->setLineWrapMode(QTextBrowser::LineWrapMode::NoWrap);
}

void MainWindow::on_checkBoxEnableTracer_toggled(bool checked)
{
    if (checked)
    {
        createChartTracer();
        connect(ui->widgetChart, SIGNAL(mouseMove(QMouseEvent *)), this, SLOT(showPointValue(QMouseEvent *)));
    }
    else
    {
        disconnect(ui->widgetChart, SIGNAL(mouseMove(QMouseEvent *)), this, SLOT(showPointValue(QMouseEvent *)));
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
    ui->comboBoxSend->addItem(ui->comboBoxSend->currentText()); // dodajemy do historii (bez tego nie dodaje)
    comboBoxSendReturnPressedSlot();
}

void MainWindow::on_pushButtonClearHistory_clicked()
{
    ui->comboBoxSend->clear();
}

void MainWindow::on_comboBoxGraphDisplayMode_currentIndexChanged(const QString &arg1)
{
    clearGraphs(true);
    loadFromRAM(0);

    QPalette *paletteRed = new QPalette();
    QPalette *paletteBlack = new QPalette();
    paletteRed->setColor(QPalette::Text, Qt::GlobalColor::red);
    paletteBlack->setColor(QPalette::Text, Qt::GlobalColor::black);

    if (arg1.contains("Auto", Qt::CaseSensitivity::CaseInsensitive))
    {
        ui->lineEditCustomParsingRules->setEnabled(false);
        ui->labelParsingRules->setEnabled(false);
        ui->comboBoxGraphDisplayMode->setPalette(*paletteBlack);
    }
    else if (arg1.contains("Custom", Qt::CaseSensitivity::CaseInsensitive))
    {
        ui->lineEditCustomParsingRules->setEnabled(true);
        ui->labelParsingRules->setEnabled(true);
        ui->comboBoxGraphDisplayMode->setPalette(*paletteRed);
    }
}

void MainWindow::on_pushButtonClearGraphs_clicked()
{
    clearGraphData(true);
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

void MainWindow::on_lineEditCustomParsingRules_editingFinished()
{
    ui->lineEditCustomParsingRules->setText(ui->lineEditCustomParsingRules->text().replace(" && ", " "));
    ui->lineEditCustomParsingRules->setText(ui->lineEditCustomParsingRules->text().simplified().replace(" ", " && "));
    clearGraphs(true);
    loadFromRAM(false);
}

void MainWindow::on_spinBoxMaxGraphs_valueChanged(int arg1)
{
    while (ui->widgetChart->graphCount() > arg1)
        ui->widgetChart->removeGraph(ui->widgetChart->graph());

    QPalette *paletteRed = new QPalette();
    QPalette *paletteBlack = new QPalette();
    paletteRed->setColor(QPalette::Text, Qt::GlobalColor::red);
    paletteBlack->setColor(QPalette::Text, Qt::GlobalColor::black);

    if (arg1 > 10)
        ui->spinBoxMaxGraphs->setPalette(*paletteRed);
    else
        ui->spinBoxMaxGraphs->setPalette(*paletteBlack);

    loadFromRAM(false);

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
    serialStringProcessingTimer->setInterval(arg1);
    udpStringProcessingTimer->setInterval(arg1);

    QPalette *paletteRed = new QPalette();
    QPalette *paletteBlack = new QPalette();
    paletteRed->setColor(QPalette::Text, Qt::GlobalColor::red);
    paletteBlack->setColor(QPalette::Text, Qt::GlobalColor::black);

    if (arg1 > ui->spinBoxProcessingDelay->minimum())
        ui->spinBoxProcessingDelay->setPalette(*paletteRed);
    else
        ui->spinBoxProcessingDelay->setPalette(*paletteBlack);
}

void MainWindow::on_comboBoxTracerStyle_currentIndexChanged(const QString &arg1)
{
    if (ui->checkBoxEnableTracer->isChecked())
    {
        if (phaseTracer != nullptr)
        {
            disconnect(ui->widgetChart, SIGNAL(mouseMove(QMouseEvent *)), this, SLOT(showPointValue(QMouseEvent *)));
            delete phaseTracer;
            phaseTracer = nullptr;
        }

        createChartTracer();
        connect(ui->widgetChart, SIGNAL(mouseMove(QMouseEvent *)), this, SLOT(showPointValue(QMouseEvent *)));

        ui->widgetChart->replot();
    }
}

void MainWindow::on_pushButtonSerialConnect_toggled(bool checked)
{
    if (checked)
    {
        if (serial.getAvailiblePortsCount() < 1)
        {
            addLog(">>\t No devices available");
            addLog(">>\t Unable to open serial port!");
            ui->pushButtonSerialConnect->setChecked(false);
            return;
        }

        // clearGraphData(true);

        QString parsedPortName = ui->comboBoxDevices->currentText().mid(ui->comboBoxDevices->currentText().indexOf("COM"), ui->comboBoxDevices->currentText().indexOf(")") - 1);
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
                fileLogger.beginLog(ui->lineEditSaveLogPath->text(), ui->checkBoxAutoLogging->isChecked(), ui->lineEditSaveFileName->text());

            connect(serialStringProcessingTimer, SIGNAL(timeout()), this, SLOT(processSerial()));

            addLog(">>\t Serial port opened. " + serial.getSerialInfo() + " DTR: " + QString::number(ui->checkBoxDTR->isChecked()));
            ui->pushButtonSerialConnect->setText("Disconnect");
        }
        else
        {
            ui->pushButtonSerialConnect->setChecked(false);
            addLog(">>\t Unable to open serial port!");
        }
    }
    else
    {
        serialStringProcessingTimer->stop();

        disconnect(serialStringProcessingTimer, SIGNAL(timeout()), this, SLOT(processSerial()));

        if (serial.end())
        {
            addLog(">>\t Connection closed.");
            ui->pushButtonSerialConnect->setText("Connect");
        }
        else
        {
            ui->pushButtonSerialConnect->setChecked(true);
            addLog(">>\t ERROR: Unable to close cennection !");
        }

        if (!ui->pushButtonSerialConnect->isChecked() && !ui->pushButtonUDPConnect->isChecked())
            fileLogger.closeFile();
    }
}

void MainWindow::on_actionSave_As_triggered()
{
    qDebug() << "Click save file !";
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Log"), "", tr("(*.txt);;All Files (*)"));

    if (fileName.isEmpty())
    {
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
    printChangeLog();
}

void MainWindow::on_actionQuit_triggered()
{
    this->close();
}

void MainWindow::on_actionPlotter_triggered()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::on_action3D_orientation_triggered()
{
    //  ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::printPlot(QPrinter *printer)
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

void MainWindow::printLog(QPrinter *printer)
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

void MainWindow::on_actionPrint_Graph_triggered()
{
    QPrinter chartPrinter;
    chartPrinter.setFullPage(true);

    QPrintPreviewDialog previewDialog(&chartPrinter);
    previewDialog.setWindowFlags(Qt::Window);

    connect(&previewDialog, SIGNAL(paintRequested(QPrinter *)), SLOT(printPlot(QPrinter *)));
    previewDialog.exec();
}

void MainWindow::on_actionPrint_log_triggered()
{
    QPrinter logPrinter;

    QPrintPreviewDialog previewLogDialog(&logPrinter);
    previewLogDialog.setWindowFlags(Qt::Window);

    connect(&previewLogDialog, SIGNAL(paintRequested(QPrinter *)), SLOT(printLog(QPrinter *)));
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
                fileLogger.beginLog(ui->lineEditSaveLogPath->text(), ui->checkBoxAutoLogging->isChecked(), ui->lineEditSaveFileName->text());

            connect(udpStringProcessingTimer, SIGNAL(timeout()), this, SLOT(processUDP()));

            addLog(">>\t UDP port opened.");

            ui->pushButtonUDPConnect->setText("Close Connection");
        }
        else
        {
            addLog(">>\t UDP error. Unable to bind");
        }
    }
    else
    {
        if (networkUDP.end())
        {
            udpStringProcessingTimer->stop();

            addLog(">>\t UDP port closed.");

            disconnect(udpStringProcessingTimer, SIGNAL(timeout()), this, SLOT(processUDP()));

            if (!ui->pushButtonSerialConnect->isChecked() && !ui->pushButtonUDPConnect->isChecked())
                fileLogger.closeFile();

            ui->pushButtonUDPConnect->setText("Open Connection");
        }
    }
}

void MainWindow::sendDatagram(QString message)
{
    if (!networkUDP.isOpen())
    {
        addLog("UDP\t Unable to send. Port closed.");
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

    addLog("UDP >>\t" + message);
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
            addLog("Logger - Filename not specified !");
            ui->pushButtonLogging->setChecked(false);
            return;
        }

        ui->pushButtonLogging->setText("Disable Logging");

        if (ui->pushButtonSerialConnect->isChecked() || ui->pushButtonUDPConnect->isChecked())
        {
            if (!fileLogger.beginLog(ui->lineEditSaveLogPath->text(), ui->checkBoxAutoLogging->isChecked(), ui->lineEditSaveFileName->text()))
            {
                addLog("Logger - Unable to open File");
                ui->pushButtonLogging->setChecked(false);
                return;
            }
        }
    }
    else
    {
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

        ui->pushButtonLogging->setText("Auto");
        ui->lineEditSaveFileName->setText("%DateTime%_Log.txt");
        ui->pushButtonLogging->setChecked(true); // Trigger begin log
    }
    else
    {
        ui->pushButtonLogging->setEnabled(true);
        ui->lineEditSaveFileName->setEnabled(true);
        ui->pushButtonAddDateTime->setEnabled(true);

        ui->pushButtonLogging->setChecked(false);
        ui->pushButtonLogging->setText("Start Logging");

        ui->lineEditSaveFileName->clear();
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

    setSelectedLabels(&tableLabels);
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
    qDebug() << "pos: " + QString::number(pos);

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

void MainWindow::on_checkBoxExternalTimeReference_toggled(bool checked)
{
    if (checked)
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

void MainWindow::on_lineEditExternalClockLabel_editingFinished()
{
    ui->lineEditExternalClockLabel->setText(ui->lineEditExternalClockLabel->text().simplified());
}

void MainWindow::on_pushButtonLoadPath_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open Log File:"), "",
                                                    tr("(*.txt);;All Files (*)"));

    ui->lineEditLoadFilePath->setText(fileName);
    QFile file(fileName);

    getFileTimeRange(&file);

    QString info = "Size: " + QString::number(file.size());
    ui->lineEditFileInfo->setText(info);
}

void MainWindow::getFileTimeRange(QFile *file)
{
    if (file->open(QIODevice::ReadOnly))
    {
        QString allData = file->readAll();
        file->close();

        QStringList readFileSplitLines = allData.split(QRegExp("[\n\r]"), QString::SplitBehavior::SkipEmptyParts);

        for (auto i = 0; i < readFileSplitLines.count(); ++i)
        {
            QStringList inputStringSplitArrayTopLine = readFileSplitLines[i].simplified().split(QRegExp("\\s+"), QString::SplitBehavior::SkipEmptyParts);                                     // rozdzielamy traktująac spacje jako separator
            QStringList inputStringSplitArrayButtomLine = readFileSplitLines[readFileSplitLines.count() - 1 - i].simplified().split(QRegExp("\\s+"), QString::SplitBehavior::SkipEmptyParts); // rozdzielamy traktująac spacje jako separator
            QStringList searchTimeFormatList = {"hh:mm:ss:zzz", "hh:mm:ss.zzz", "hh:mm:ss.z"};

            bool foundTime[2] = {false};

            for (auto j = 0; j < inputStringSplitArrayTopLine.count(); ++j)
            {
                foreach (auto timeFormat, searchTimeFormatList)
                {
                    if (QTime::fromString(inputStringSplitArrayTopLine[j], timeFormat).isValid())
                    {
                        ui->timeEditMinParsingTime->setTime(QTime::fromString(inputStringSplitArrayTopLine[j], timeFormat));
                        foundTime[0] = true;
                        break;
                    }
                }
            }

            for (auto j = 0; j < inputStringSplitArrayButtomLine.count(); ++j)
            {
                foreach (auto timeFormat, searchTimeFormatList)
                {
                    if (QTime::fromString(inputStringSplitArrayButtomLine[j], timeFormat).isValid())
                    {
                        ui->timeEditMaxParsingTime->setTime(QTime::fromString(inputStringSplitArrayButtomLine[j], timeFormat));
                        foundTime[1] = true;

                        break;
                    }
                }
            }

            if (foundTime[0] && foundTime[1])
                return;
        }
    }
    else
    {
        qDebug() << "Get file time range - invalid file ?";
        ui->timeEditMaxParsingTime->setTime(QTime());
        ui->timeEditMinParsingTime->setTime(QTime());
    }
}

void MainWindow::on_pushButtonFitToContents_clicked()
{
    ui->widgetChart->rescaleAxes(true);
    ui->widgetChart->yAxis->scaleRange(1.20);
}

void MainWindow::processLoadedFileLine(QString *line, int *progressPercent)
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

void MainWindow::updateProgressBar(float *percent)
{
    ui->progressBarLoadFile->setValue(*percent);
}

void MainWindow::processLoadedFile(QString *text)
{
    connect(&parser, SIGNAL(updateProgress(float *)), this, SLOT(updateProgressBar(float *)));
    parser.setReportProgress(true);

    parser.clearStorage();
    parser.setParsingTimeRange(ui->timeEditMinParsingTime->time(), ui->timeEditMaxParsingTime->time());
    parser.parse(*text, false, true, "");
    parser.resetTimeRange();

    parser.setReportProgress(false);
    disconnect(&parser, SIGNAL(updateProgress(float *)), this, SLOT(updateProgressBar(float *)));

    QStringList labelList = parser.getStringListLabels();
    QList<double> numericDataList = parser.getListNumericValues();
    QList<long> timeStampList = parser.getListTimeStamp();

    this->processChart(labelList, numericDataList, timeStampList);
    this->processTable(labelList, numericDataList);
    this->saveToRAM(labelList, numericDataList, timeStampList);

    disconnect(&fileReader, SIGNAL(textReady(QString *)), this, SLOT(processLoadedFile(QString *)));
}

void MainWindow::on_pushButtonLoadRAMBuffer_clicked()
{
    if (ui->checkBoxRAMClearChart->isChecked())
        this->clearGraphData(false);

    loadFromRAM((bool)ui->comboBoxRAMLoadMode->currentIndex());
}

void MainWindow::saveToRAM(QStringList newlabelList, QList<double> newDataList, QList<long> newTimeList, bool saveText, QString text)
{
    if (ui->checkBoxEnableRAMBuffer->isChecked() == false)
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
            addLog(line);
    }

    if (RAMLabels.isEmpty() || RAMData.isEmpty() || RAMTime.isEmpty())
        return;

    processChart(RAMLabels, RAMData, RAMTime);
}

void MainWindow::on_pushButtonRAMClear_clicked()
{
    parser.clearStorage();
}

void MainWindow::on_checkBoxEnableRAMBuffer_toggled(bool checked)
{
    parser.clearStorage();
    ui->pushButtonSaveParserMemory->setEnabled(!checked);
}

void MainWindow::on_checkBoxSyncSystemClock_toggled(bool checked)
{
    // clearGraphs(true);
    ui->checkBoxExternalTimeReference->setChecked(false);
    ui->checkBoxExternalTimeReference->setEnabled(!checked);
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
            connect(&fileReader, SIGNAL(textReady(QString *)), this, SLOT(processLoadedFile(QString *)));

            connect(&fileReader, &FileReader::fileReadFinished, this, [=]() {
                ui->pushButtonLoadFile->setText("Load File");
                ui->progressBarLoadFile->setValue(100);
            });

            if (fileReader.readAllAtOnce(&inputFile))
            {
                addLog("Read file succesfully... ");
            }
            else
            {
                addLog("invalid file !");
                ui->pushButtonLoadFile->setText("Load File");
                ui->progressBarLoadFile->setValue(0);
            }
        }
        else
        {
            addLog("File Reader - Invalid file path !");
            ui->pushButtonLoadFile->setText("Load File");
            ui->progressBarLoadFile->setValue(0);
        }
    }
    else
    {
        parser.abort();
        disconnect(&fileReader, SIGNAL(textReady(QString *)), this, SLOT(processLoadedFile(QString *)));
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

void MainWindow::on_actionUser_guide_triggered()
{
    QWhatsThis::enterWhatsThisMode();
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
