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
    on_spinBoxProcessingDelay_valueChanged(ui->spinBoxProcessingDelay->value()); // apply even if unchanged from the .ui default
    setupToolsMenu();
    setupThemeControls();
    setupToolBar();
    applyTheme();
    applyOpenGl(); // saved preferences
    applyChartEngine();

    infoDialog.setFixedSize(800, 600);

    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(on_aboutToQuitSlot()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::handleSerialConnectionLost(const QString &reason)
{
    if (waitingForReconnect || !serial.isOpen())
        return; // errors keep coming while the device is gone

    addLog("App >>\t Serial port error: " + reason, true);

    if (!ui->checkBoxAutoReconnect->isChecked())
    {
        ui->pushButtonSerialConnect->setChecked(false); // closes the port via toggled slot
        return;
    }

    serialPipeline.flush();
    serial.end();

    waitingForReconnect = true;
    reconnectTimer.start();
    ui->pushButtonSerialConnect->setText("Reconnecting... (click to cancel)");
    addLog("App >>\t Waiting for " + connectedPort.portName + " to come back...", true);
}

void MainWindow::tryReconnect()
{
    const QString portName = PortIdentity::findMatch(connectedPort, knownPorts);
    if (portName.isEmpty())
        return;

    // The device can take a moment before it accepts connections - just retry on the next tick.
    if (!openSerialPort(portName))
        return;

    stopReconnecting();
    refreshDeviceList();
    const int index = ui->comboBoxDevices->findData(portName);
    if (index >= 0)
        ui->comboBoxDevices->setCurrentIndex(index);
    addLog("App >>\t Reconnected to " + portName + ".", true);
}

void MainWindow::stopReconnecting()
{
    waitingForReconnect = false;
    reconnectTimer.stop();
}

void MainWindow::startDemo(int rateHz)
{
    demoRate = rateHz;
    demoListed = true;
    refreshDeviceList();

    const int index = ui->comboBoxDevices->findData(demoPortId);
    if (index < 0)
        return;

    ui->comboBoxDevices->setCurrentIndex(index);
    ui->tabWidgetControlSection->setCurrentIndex(TAB_INDEX::SERIAL_USB);
    ui->pushButtonEnablePlot->setChecked(false); // checked == plotting disabled
    ui->checkBoxAutoTrack->setChecked(true);

    if (ui->pushButtonSerialConnect->isChecked())
        ui->pushButtonSerialConnect->setChecked(false);
    ui->pushButtonSerialConnect->setChecked(true);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (serial.isOpen() || networkUDP.isOpen() || demo.isOpen())
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
    settingsSaveAll();
}

void MainWindow::createTimers()
{
    this->serialDeviceCheckTimer = new QTimer(this);
    this->radioButtonTimer = new QTimer(this);

    connect(serialDeviceCheckTimer, SIGNAL(timeout()), this, SLOT(on_updateSerialDeviceList()));
    connect(&fileReader, &FileReader::progress, ui->progressBarLoadFile, &QProgressBar::setValue);
    connect(&fileReader, &FileReader::loaded, this, &MainWindow::handleFileLoaded);

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &MainWindow::applyTheme);

    connectTransport(&serial, &serialPipeline);
    connectTransport(&demo, &serialPipeline); // demo stands in for a serial device
    connectTransport(&networkUDP, &udpPipeline);

    connect(&serial, &Transport::connectionLost, this, &MainWindow::handleSerialConnectionLost, Qt::QueuedConnection);

    chartRefreshTimer.setSingleShot(true);
    connect(&chartRefreshTimer, &QTimer::timeout, this, &MainWindow::refreshChart);
    setChartRefreshRate(preferences.chartRefreshRate);

    reconnectTimer.setInterval(500);
    connect(&reconnectTimer, &QTimer::timeout, &portScanner, &PortScanner::scan); // result -> handlePortScan -> tryReconnect
    connect(&portScanner, &PortScanner::scanned, this, &MainWindow::handlePortScan);
    connect(radioButtonTimer, &QTimer::timeout, this, [this]()
            { ui->radioButtonDeviceUpdate->setChecked(false); });
}

void MainWindow::setupOrientationView()
{
    auto *page = new QWidget(this);
    page->setObjectName("pageOrientation");
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    orientationView = new OrientationView(page);
    layout->addWidget(orientationView, 1);

    auto makeLabelEdit = [page](const char *name, const QString &text, const QString &tip)
    {
        auto *edit = new QLineEdit(text, page);
        edit->setObjectName(name);
        edit->setToolTip(tip);
        edit->setMaximumWidth(110);
        return edit;
    };
    auto *roll = makeLabelEdit("lineEditOrientationRoll", "roll", "Label whose value is the roll angle (right side down = positive)");
    auto *pitch = makeLabelEdit("lineEditOrientationPitch", "pitch", "Label whose value is the pitch angle (nose up = positive)");
    auto *yaw = makeLabelEdit("lineEditOrientationYaw", "yaw", "Label whose value is the yaw / heading (clockwise from above = positive)");
    auto *radians = new QCheckBox("Values in radians", page);
    radians->setObjectName("checkBoxOrientationRadians");
    auto *back = new QPushButton("Back to chart", page);
    back->setObjectName("pushButtonOrientationBack");
    connect(back, &QPushButton::clicked, this, &MainWindow::on_actionPlotter_triggered);

    auto *controls = new QHBoxLayout;
    controls->addWidget(new QLabel("Roll:", page));
    controls->addWidget(roll);
    controls->addWidget(new QLabel("Pitch:", page));
    controls->addWidget(pitch);
    controls->addWidget(new QLabel("Yaw:", page));
    controls->addWidget(yaw);
    controls->addWidget(radians);
    controls->addStretch(1);
    controls->addWidget(back);
    layout->addLayout(controls);

    ui->stackedWidgetGraphView->addWidget(page);
    ui->action3D_orientation->setEnabled(true);

    // The demo device adds roll/pitch/yaw while the 3D view is shown.
    connect(ui->stackedWidgetGraphView, &QStackedWidget::currentChanged, this, [this, page](int)
            {
                const bool orientationShown = ui->stackedWidgetGraphView->currentWidget() == page;
                demo.setOrientationOutput(orientationShown);
                if (orientationShown)
                {
                    orientationDirty = true;
                    requestChartRefresh();
                }
            });
}

// Keeps the latest roll / pitch / yaw from parsed data; the view is updated on the refresh tick.
void MainWindow::updateOrientation(const QStringList &labels, const QList<double> &values)
{
    const QString names[3] = {findChild<QLineEdit *>("lineEditOrientationRoll")->text().trimmed(),
                              findChild<QLineEdit *>("lineEditOrientationPitch")->text().trimmed(),
                              findChild<QLineEdit *>("lineEditOrientationYaw")->text().trimmed()};
    bool changed = false;
    const auto count = qMin(labels.count(), values.count());
    for (auto i = 0; i < count; ++i)
    {
        for (int axis = 0; axis < 3; ++axis)
        {
            if (!names[axis].isEmpty() && labels[i].compare(names[axis], Qt::CaseInsensitive) == 0)
            {
                orientationValues[axis] = values[i];
                orientationReceived = true;
                changed = true;
            }
        }
    }

    if (changed)
    {
        orientationDirty = true;
        requestChartRefresh();
    }
}

void MainWindow::refreshOrientationView()
{
    orientationDirty = false;
    if (!orientationReceived)
    {
        orientationView->setHasData(false);
        return;
    }

    const bool radians = findChild<QCheckBox *>("checkBoxOrientationRadians")->isChecked();
    auto degrees = [radians](double value) { return radians ? qRadiansToDegrees(value) : value; };
    orientationView->setOrientation(degrees(orientationValues[0]), degrees(orientationValues[1]), degrees(orientationValues[2]));
}

// An "Options" button with a popup panel below (or above, near the screen bottom).
QFrame *MainWindow::makeOptionsPopup(QToolButton *button, const QString &objectName)
{
    button->setText("Options");
    button->setArrowType(Qt::DownArrow);
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    auto *panel = new QFrame(this, Qt::Popup);
    panel->setObjectName(objectName);
    panel->setProperty("optionsPopup", true); // styled by the theme
    panel->setFrameShape(QFrame::StyledPanel);

    connect(button, &QToolButton::clicked, this, [button, panel]()
            {
                panel->adjustSize();
                QPoint position = button->mapToGlobal(QPoint(button->width() - panel->width(), button->height()));
                if (const QScreen *screen = button->screen())
                {
                    const QRect available = screen->availableGeometry();
                    if (position.y() + panel->height() > available.bottom())
                        position.setY(button->mapToGlobal(QPoint(0, 0)).y() - panel->height());
                    position.setX(qBound(available.left(), position.x(), available.right() - panel->width()));
                }
                panel->move(position);
                panel->show();
            });
    return panel;
}

// Same idea for the chart: the three rows of options under the chart become part of the Pause row
// plus an "Options" popup, so the chart gets the height.
void MainWindow::setupChartOptions()
{
    // ----- Row under the chart (next to Pause and the history scroll bar) ----- //
    ui->checkBoxAutoTrack->setText("Auto-scroll");
    ui->checkBoxAutoTrack->setToolTip("Follow the newest data (X on the chart)");
    ui->spinBoxScrollingTimeRange->setPrefix("Last ");
    ui->spinBoxScrollingTimeRange->setSuffix(" s");
    ui->spinBoxScrollingTimeRange->setToolTip("Visible time window while auto-scrolling");
    ui->pushButtonClearGraphs->setToolTip("Remove the plotted data, keep the graphs");

    auto *optionsButton = new QToolButton(this);
    optionsButton->setObjectName("toolButtonChartOptions");
    optionsButton->setToolTip("Chart options");

    chartBarLayout->addWidget(ui->checkBoxAutoTrack);
    chartBarLayout->addWidget(ui->spinBoxScrollingTimeRange);
    chartBarLayout->addWidget(ui->pushButtonClearGraphs);
    ui->toolButtonAdvancedGraphMenu->setMinimumWidth(0); // sized for the old grid
    ui->toolButtonAdvancedGraphMenu->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    chartBarLayout->addWidget(ui->toolButtonAdvancedGraphMenu);
    chartBarLayout->addWidget(optionsButton);

    // ----- Options popup ----- //
    QFrame *panel = makeOptionsPopup(optionsButton, "chartOptionsPanel");
    auto *panelLayout = new QVBoxLayout(panel);

    ui->checkBoxEnableTracer->setText("Tracer");
    ui->checkBoxAutoRescaleY->setText("Auto-scale Y");
    ui->checkBoxAutoRescaleY->setToolTip("Fit the value axis to the data while auto-scrolling");
    ui->pushButtonEnablePlot->setToolTip("Stop adding new data to the chart (the text view and logging continue)");
    on_pushButtonEnablePlot_toggled(ui->pushButtonEnablePlot->isChecked()); // new button texts

    auto *tracerRow = new QHBoxLayout;
    tracerRow->addWidget(ui->checkBoxEnableTracer);
    tracerRow->addWidget(ui->comboBoxTracerStyle, 1);

    auto *viewGroup = new QGroupBox("View", panel);
    auto *viewForm = new QFormLayout(viewGroup);
    viewForm->addRow(ui->checkBoxShowLegend);
    viewForm->addRow(tracerRow);
    viewForm->addRow(ui->checkBoxAutoRescaleY);
    viewForm->addRow("Plotting:", ui->pushButtonEnablePlot);

    ui->spinBoxMaxGraphs->setPrefix(QString());
    ui->spinBoxMaxGraphs->setSuffix(QString());
    ui->spinBoxMaxTimeRange->setPrefix(QString());
    ui->spinBoxMaxTimeRange->setSuffix(" s");
    ui->spinBoxMaxTimeRange->setSpecialValueText("forever"); // 0 = keep everything
    ui->spinBoxRemoveOldLabels->setPrefix(QString());
    ui->spinBoxRemoveOldLabels->setSuffix(" updates");
    ui->spinBoxRemoveOldLabels->setSpecialValueText("never"); // 0 = keep graphs
    ui->spinBoxRemoveOldLabels->setToolTip("Remove a graph after this many updates without new data for it");
    ui->lineEditCustomParsingRules->setMinimumWidth(180);

    auto *graphsGroup = new QGroupBox("Graphs", panel);
    auto *graphsForm = new QFormLayout(graphsGroup);
    graphsForm->addRow("Show graphs:", ui->comboBoxGraphDisplayMode);
    graphsForm->addRow("Labels:", ui->lineEditCustomParsingRules);
    graphsForm->addRow("Max graphs:", ui->spinBoxMaxGraphs);
    graphsForm->addRow("Keep data for:", ui->spinBoxMaxTimeRange);
    graphsForm->addRow("Remove silent graph after:", ui->spinBoxRemoveOldLabels);

    auto *clockRow = new QHBoxLayout;
    clockRow->addWidget(ui->lineEditExternalClockLabel, 1);
    clockRow->addWidget(ui->comboBoxExternalTimeFormat);

    auto *timeGroup = new QGroupBox("Time", panel);
    auto *timeForm = new QFormLayout(timeGroup);
    timeForm->addRow("Time reference:", ui->comboBoxClockSource);
    timeForm->addRow("External clock:", clockRow);

    panelLayout->addWidget(viewGroup);
    panelLayout->addWidget(graphsGroup);
    panelLayout->addWidget(timeGroup);
    panelLayout->addWidget(ui->pushButtonClearAll);

    // The old panel is now empty.
    ui->scrollArea->hide();
}

// Replaces the crowded, horizontally scrolling options area under the text view with one row of
// frequently used actions and an "Options" popup for the rest. The existing widgets are re-parented,
// so their slots and saved settings are unchanged.
void MainWindow::setupTextViewOptions()
{
    // ----- Row under the text view ----- //
    auto *bar = new QWidget(this);
    bar->setObjectName("textViewBar");
    auto *row = new QHBoxLayout(bar);
    row->setContentsMargins(0, 4, 0, 0);

    ui->pushButtonTextLogToggle->setText("Pause text");
    ui->pushButtonTextLogToggle->setToolTip("Stop adding received text to the view (plotting and logging continue)");
    ui->checkBoxScrollToButtom->setText("Auto-scroll");
    ui->pushButtonSerialLogScrollown->setText("Newest");
    ui->pushButtonSerialLogScrollown->setToolTip("Scroll to the newest line");
    ui->pushButtonClear->setToolTip("Clear the text view");
    ui->lineEditHighlight->setPlaceholderText("Highlight text (Enter)");
    ui->lineEditHighlight->setClearButtonEnabled(true);
    ui->lineEditHighlight->setMinimumWidth(80);

    auto *optionsButton = new QToolButton(bar);
    optionsButton->setObjectName("toolButtonTextViewOptions");
    optionsButton->setToolTip("Text view options");

    row->addWidget(ui->pushButtonTextLogToggle);
    row->addWidget(ui->checkBoxScrollToButtom);
    row->addWidget(ui->pushButtonSerialLogScrollown);
    row->addWidget(ui->pushButtonClear);
    row->addWidget(ui->lineEditHighlight, 1);
    row->addWidget(optionsButton);

    // ----- Options popup ----- //
    QFrame *panel = makeOptionsPopup(optionsButton, "textViewOptionsPanel");
    auto *panelLayout = new QVBoxLayout(panel);

    ui->checkBoxShowTime->setText("Timestamps");
    ui->checkBoxWrapText->setText("Wrap long lines");
    ui->checkBoxShowControlChars->setText("Show control characters (\\r \\n \\t, spaces as dots)");
    ui->checkBoxSendKey->setText("Send key presses typed in the text view");
    ui->spinBoxMaxLines->setPrefix(QString());
    ui->spinBoxMaxLines->setSuffix(" lines");
    ui->spinBoxProcessingDelay->setPrefix(QString());
    ui->spinBoxProcessingDelay->setSuffix(" ms");
    ui->spinBoxProcessingDelay->setToolTip("Received data is collected for this long before the view is updated");

    auto *displayGroup = new QGroupBox("Display", panel);
    auto *displayForm = new QFormLayout(displayGroup);
    displayForm->addRow("Format:", ui->comboBoxFormat);
    displayForm->addRow("New data:", ui->comboBoxAddTextMode);
    displayForm->addRow("Whitespace:", ui->comboBoxTextProcessing);
    displayForm->addRow("Show:", ui->comboBoxMessagesDisplayMode);
    displayForm->addRow("Keep at most:", ui->spinBoxMaxLines);
    displayForm->addRow(ui->checkBoxShowTime);
    displayForm->addRow(ui->checkBoxWrapText);
    displayForm->addRow(ui->checkBoxShowControlChars);

    auto *inputGroup = new QGroupBox("Input", panel);
    auto *inputForm = new QFormLayout(inputGroup);
    inputForm->addRow(ui->checkBoxSendKey);
    inputForm->addRow("Processing delay:", ui->spinBoxProcessingDelay);

    panelLayout->addWidget(displayGroup);
    panelLayout->addWidget(inputGroup);

    // The old area is now empty.
    const int index = ui->verticalLayout_2->indexOf(ui->scrollAreaTextLogOptions);
    ui->scrollAreaTextLogOptions->hide();
    ui->verticalLayout_2->insertWidget(index, bar);
}

void MainWindow::setupGUI()
{
    setupOrientationView();
    setupTextViewOptions();
    setupChartOptions();

    // ----------------------- MainWindow ----------------------- //
    {
        connect(ui->comboBoxSend->lineEdit(), SIGNAL(returnPressed()), this, SLOT(on_comboBoxSendReturnPressedSlot()));
        ui->comboBoxSend->setInsertPolicy(QComboBox::NoInsert); // history is managed by CommandHistory
        // Key presses arrive at the combo box (its line edit forwards focus to it), so filter both;
        // otherwise QComboBox's own Up/Down would step through the list in the wrong direction.
        ui->comboBoxSend->installEventFilter(this);
        ui->comboBoxSend->lineEdit()->installEventFilter(this);
        ui->comboBoxSend->lineEdit()->setPlaceholderText("Type a command, Enter to send, Up/Down for history");

        // Periodic send
        checkBoxRepeatSend = new QCheckBox("Repeat every", this);
        checkBoxRepeatSend->setObjectName("checkBoxRepeatSend");
        checkBoxRepeatSend->setToolTip("Send the text periodically until unchecked or the connection closes.");
        spinBoxRepeatInterval = new QSpinBox(this);
        spinBoxRepeatInterval->setObjectName("spinBoxRepeatInterval");
        spinBoxRepeatInterval->setRange(10, 3600000);
        spinBoxRepeatInterval->setValue(1000);
        spinBoxRepeatInterval->setSuffix(" ms");
        ui->gridLayout_3->addWidget(checkBoxRepeatSend, 1, 6);
        ui->gridLayout_3->addWidget(spinBoxRepeatInterval, 1, 7);
        connect(checkBoxRepeatSend, &QCheckBox::toggled, this, &MainWindow::setRepeatSend);
        connect(spinBoxRepeatInterval, QOverload<int>::of(&QSpinBox::valueChanged), &repeatSendTimer, QOverload<int>::of(&QTimer::setInterval));
        connect(&repeatSendTimer, &QTimer::timeout, this, [this]()
                {
                    if (!sendText(repeatSendText))
                        checkBoxRepeatSend->setChecked(false);
                });

        // Macros
        macroBar = new MacroBar(this);
        macroBar->setObjectName("macroBar");
        ui->gridLayout_3->addWidget(macroBar, 2, 1, 1, 7);
        connect(macroBar, &MacroBar::sendRequested, this, [this](const QString &text)
                { sendText(text); });
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
    ui->lineEditSaveLogPath->setText(defaultLogPath());
    ui->splitterGraphTable->setSizes({this->height(), 0});

    emit on_checkBoxAutoLogging_toggled(ui->checkBoxAutoLogging->isChecked());
    emit on_checkBoxShowLegend_toggled(ui->checkBoxShowLegend->isChecked());
    emit on_comboBoxClockSource_currentIndexChanged(ui->comboBoxClockSource->currentIndex());

    on_updateSerialDeviceList();
}

void MainWindow::createChart()
{
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
    QFont legendFont = font();
    legendFont.setPointSizeF(legendFont.pointSizeF() * 0.9);
    ui->widgetChart->legend->setFont(legendFont);
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


    if (ui->checkBoxEnableTracer->isChecked())
    {
        createChartTracer();
        connect(ui->widgetChart, SIGNAL(mouseMove(QMouseEvent *)), this, SLOT(on_tracerShowPointValue(QMouseEvent *)));
    }

    // ----- Pause button + history scroll bar under the chart ----- //
    pushButtonPauseChart = new QPushButton("Pause", this);
    pushButtonPauseChart->setObjectName("pushButtonPauseChart");
    pushButtonPauseChart->setCheckable(true);
    pushButtonPauseChart->setToolTip("Freeze the chart while data keeps being recorded (Space or P on the chart).");

    scrollBarChartHistory = new QScrollBar(Qt::Horizontal, this);
    scrollBarChartHistory->setObjectName("scrollBarChartHistory");
    scrollBarChartHistory->setToolTip("Scroll through the recorded history. Moving it pauses the chart.");

    chartBarLayout = new QHBoxLayout;
    auto *historyRow = chartBarLayout;
    historyRow->addWidget(pushButtonPauseChart);
    historyRow->addWidget(scrollBarChartHistory, 1);
    // Insert directly under the chart; the chart options panel moves one row down.
    int row = 0, column = 0, rowSpan = 1, columnSpan = 1;
    const int panelIndex = ui->gridLayout_12->indexOf(ui->scrollArea);
    ui->gridLayout_12->getItemPosition(panelIndex, &row, &column, &rowSpan, &columnSpan);
    ui->gridLayout_12->removeWidget(ui->scrollArea);
    ui->gridLayout_12->addLayout(historyRow, row, column, 1, columnSpan);
    ui->gridLayout_12->addWidget(ui->scrollArea, row + 1, column, rowSpan, columnSpan);

    connect(pushButtonPauseChart, &QPushButton::toggled, this, &MainWindow::setChartPaused);
    connect(scrollBarChartHistory, &QScrollBar::valueChanged, this, &MainWindow::scrollChartHistory);
    connect(ui->widgetChart->xAxis, QOverload<const QCPRange &>::of(&QCPAxis::rangeChanged), this, [this](const QCPRange &)
            { updateHistoryScrollBar(); });

    ui->textBrowserLogs->setHighlightEnabled(false);
    emit on_comboBoxLogFormat_currentIndexChanged(ui->comboBoxLogFormat->currentIndex());
}

void MainWindow::setupThemeControls()
{
    // View -> Theme menu, shared with the theme button in the menu bar corner.
    QMenu *themeMenu = ui->menuView->addMenu("Theme");
    themeActions = new QActionGroup(this);
    for (auto mode : {ThemeManager::Mode::System, ThemeManager::Mode::Light, ThemeManager::Mode::Dark})
    {
        QAction *action = themeMenu->addAction(ThemeManager::displayName(mode));
        action->setCheckable(true);
        action->setData(int(mode));
        action->setChecked(mode == ThemeManager::instance().mode());
        themeActions->addAction(action);
    }
    connect(themeActions, &QActionGroup::triggered, this, [](QAction *action)
            {
                ThemeManager::instance().apply(ThemeManager::Mode(action->data().toInt()));
                ThemeManager::instance().save();
            });

    themeButton = new QToolButton(this);
    themeButton->setObjectName("toolButtonTheme");
    themeButton->setAutoRaise(true);
    themeButton->setPopupMode(QToolButton::InstantPopup);
    themeButton->setMenu(themeMenu);
    themeButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui->menuBar->setCornerWidget(themeButton, Qt::TopRightCorner);

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &MainWindow::updateThemeButton);
    updateThemeButton();

    // The menu bar lays out its corner widget only once, so make room for the longest label up front.
    int widest = 0;
    for (auto mode : {ThemeManager::Mode::System, ThemeManager::Mode::Light, ThemeManager::Mode::Dark})
    {
        themeButton->setText(ThemeManager::displayName(mode));
        widest = qMax(widest, themeButton->sizeHint().width());
    }
    themeButton->setMinimumWidth(widest);
    updateThemeButton();
}

void MainWindow::setupToolsMenu()
{
    QMenu *toolsMenu = new QMenu("Tools", this);
    toolsMenu->setObjectName("menuTools");
    ui->menuBar->insertMenu(ui->menuHelp->menuAction(), toolsMenu);

    QAction *settingsAction = toolsMenu->addAction("Settings...");
    settingsAction->setObjectName("actionSettings");
    settingsAction->setShortcut(QKeySequence::Preferences);
    settingsAction->setMenuRole(QAction::PreferencesRole); // macOS: application menu
    connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettings);
    toolsMenu->addSeparator();

    actionDemoDevice = toolsMenu->addAction("Demo device (simulated sine waves)");
    actionDemoDevice->setObjectName("actionDemoDevice");
    actionDemoDevice->setCheckable(true);
    actionDemoDevice->setToolTip("Stream simulated data without any hardware. Also available as --demo on the command line.");
    connect(actionDemoDevice, &QAction::triggered, this, [this](bool checked)
            {
                if (checked)
                    startDemo(demoRate);
                else if (demo.isOpen())
                    ui->pushButtonSerialConnect->setChecked(false);
                actionDemoDevice->setChecked(demo.isOpen());
            });
    connect(ui->pushButtonSerialConnect, &QPushButton::toggled, this, [this]()
            { actionDemoDevice->setChecked(demo.isOpen()); });
}

void MainWindow::openSettings()
{
    SettingsDialog dialog(preferences, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    applyPreferences(dialog.preferences());
    if (dialog.themeMode() != ThemeManager::instance().mode())
    {
        ThemeManager::instance().apply(dialog.themeMode());
        ThemeManager::instance().save();
    }
}

void MainWindow::applyPreferences(const AppPreferences &newPreferences)
{
    preferences = newPreferences;
    preferences.save();

    setChartRefreshRate(preferences.chartRefreshRate);
    applyOpenGl();
    applyChartEngine();
    for (auto i = 0; i < ui->widgetChart->graphCount(); ++i)
        styleGraph(ui->widgetChart->graph(i));
    ui->widgetChart->replot();
}

// Experimental: draw the chart with Qt Graphs instead of QCustomPlot (see GraphsChartView).
void MainWindow::applyChartEngine()
{
#ifdef QTSM_HAS_QT_GRAPHS
    const bool useQtGraphs = preferences.chartEngine == AppPreferences::ChartEngine::QtGraphs;
    if (useQtGraphs && !graphsChartView)
    {
        graphsChartView = new GraphsChartView(this);
        if (!graphsChartView->isReady())
        {
            addLog("App >>\t Qt Graphs could not be started (" + graphsChartView->errorString() + ") - using QCustomPlot.", true);
            delete graphsChartView;
            graphsChartView = nullptr;
            return;
        }
        int row = 0, column = 0, rowSpan = 1, columnSpan = 1;
        ui->gridLayout_12->getItemPosition(ui->gridLayout_12->indexOf(ui->widgetChart), &row, &column, &rowSpan, &columnSpan);
        // Same sizing as the QCustomPlot widget, and the chart row takes the spare height
        // whichever chart is shown (otherwise the options panel below grows instead).
        graphsChartView->setSizePolicy(ui->widgetChart->sizePolicy());
        graphsChartView->setMinimumSize(ui->widgetChart->minimumSize());
        ui->gridLayout_12->setRowStretch(row, 1);
        ui->gridLayout_12->addWidget(graphsChartView, row, column, rowSpan, columnSpan);

        connect(graphsChartView, &GraphsChartView::autoTrackOffRequested, this, [this]()
                { ui->checkBoxAutoTrack->setChecked(false); });
        connect(graphsChartView, &GraphsChartView::autoTrackToggleRequested, ui->checkBoxAutoTrack, &QCheckBox::toggle);
        connect(graphsChartView, &GraphsChartView::contextMenuRequested, this, [this](const QPoint &globalPos)
                {
                    QMenu menu(this);
                    menu.addAction("Fit all data", this, [this]()
                                   {
                                       ui->widgetChart->rescaleAxes(true);
                                       ui->widgetChart->yAxis->scaleRange(1.20);
                                       graphsChartView->sync(ui->widgetChart);
                                   });
                    menu.addSeparator();
                    menu.addAction("Show all graphs", this, [this]()
                                   {
                                       on_showAllGraphs();
                                       graphsChartView->sync(ui->widgetChart);
                                   });
                    menu.addAction("Hide all graphs", this, [this]()
                                   {
                                       on_hideAllGraphs();
                                       graphsChartView->sync(ui->widgetChart);
                                   });
                    menu.exec(globalPos);
                });
    }

    if (graphsChartView)
    {
        graphsChartView->setVisible(useQtGraphs);
        graphsChartView->applyTheme(preferences.lineQuality == AppPreferences::LineQuality::Fast);
        graphsChartView->setLegendVisible(ui->checkBoxShowLegend->isChecked());
    }
    ui->widgetChart->setVisible(!useQtGraphs || !graphsChartView);
    requestChartRefresh();
#endif
}

// Experimental: let QCustomPlot draw through OpenGL. Falls back to software drawing if no context.
void MainWindow::applyOpenGl()
{
    if (ui->widgetChart->openGl() == preferences.openGl)
        return;

    ui->widgetChart->setOpenGl(preferences.openGl, 8); // 8x multisampling for smooth lines
    if (preferences.openGl && !ui->widgetChart->openGl())
        addLog("App >>\t OpenGL could not be started - the chart uses normal drawing.", true);
}

void MainWindow::setupToolBar()
{
    QToolBar *toolBar = addToolBar("Main");
    toolBar->setObjectName("mainToolBar");
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(20, 20));
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    actionConnect = toolBar->addAction("Connect");
    actionConnect->setObjectName("actionToolbarConnect");
    actionConnect->setCheckable(true);
    actionConnect->setToolTip("Connect to the device selected in the Serial COM tab");
    connect(actionConnect, &QAction::triggered, this, [this](bool checked)
            {
                ui->tabWidgetControlSection->setCurrentIndex(TAB_INDEX::SERIAL_USB);
                ui->pushButtonSerialConnect->setChecked(checked);
                updateToolBar(); // connecting may have failed
            });
    connect(ui->pushButtonSerialConnect, &QPushButton::toggled, this, &MainWindow::updateToolBar);

    toolBar->addSeparator();

    actionPause = toolBar->addAction("Pause");
    actionPause->setObjectName("actionToolbarPause");
    actionPause->setCheckable(true);
    actionPause->setToolTip("Freeze the chart while recording continues (Space)");
    connect(actionPause, &QAction::triggered, pushButtonPauseChart, &QPushButton::setChecked);
    connect(pushButtonPauseChart, &QPushButton::toggled, this, &MainWindow::updateToolBar);

    actionClear = toolBar->addAction("Clear");
    actionClear->setObjectName("actionToolbarClear");
    actionClear->setToolTip("Clear the chart and tables");
    connect(actionClear, &QAction::triggered, ui->pushButtonClearAll, &QPushButton::click);

    toolBar->addSeparator();

    actionSaveLog = toolBar->addAction("Save log");
    actionSaveLog->setToolTip("Save the received text to a file");
    connect(actionSaveLog, &QAction::triggered, ui->actionSave_As, &QAction::trigger);

    actionExportImage = toolBar->addAction("Export chart");
    actionExportImage->setToolTip("Save the chart as PNG, JPG, BMP or PDF");
    connect(actionExportImage, &QAction::triggered, this, &MainWindow::on_actionImage_triggered);

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &MainWindow::updateToolBar);
    updateToolBar();
}

void MainWindow::updateToolBar()
{
    const QColor color = ThemeManager::instance().colors().text;
    const bool connected = ui->pushButtonSerialConnect->isChecked();

    {
        const QSignalBlocker blockConnect(actionConnect);
        actionConnect->setChecked(connected);
    }
    actionConnect->setText(connected ? "Disconnect" : "Connect");
    actionConnect->setIcon(Icons::make(connected ? Icons::Shape::Disconnect : Icons::Shape::Connect, connected ? ThemeManager::instance().colors().accentText : color));

    const bool paused = pushButtonPauseChart->isChecked();
    {
        const QSignalBlocker blockPause(actionPause);
        actionPause->setChecked(paused);
    }
    actionPause->setText(paused ? "Resume" : "Pause");
    actionPause->setIcon(Icons::make(paused ? Icons::Shape::Play : Icons::Shape::Pause, paused ? ThemeManager::instance().colors().accentText : color));
    pushButtonPauseChart->setIcon(Icons::make(paused ? Icons::Shape::Play : Icons::Shape::Pause, paused ? ThemeManager::instance().colors().accentText : color));

    actionClear->setIcon(Icons::make(Icons::Shape::Clear, color));
    actionSaveLog->setIcon(Icons::make(Icons::Shape::Save, color));
    actionExportImage->setIcon(Icons::make(Icons::Shape::Image, color));
}

void MainWindow::updateThemeButton()
{
    const auto &theme = ThemeManager::instance();
    const Icons::Shape shape = theme.mode() == ThemeManager::Mode::System ? Icons::Shape::Auto
                               : theme.colors().dark                     ? Icons::Shape::Moon
                                                                         : Icons::Shape::Sun;
    themeButton->setIcon(Icons::make(shape, theme.colors().text));
    themeButton->setText(ThemeManager::displayName(theme.mode()));
    themeButton->setToolTip("Theme: " + ThemeManager::displayName(theme.mode()) + " (click to change)");

    for (QAction *action : themeActions->actions())
        action->setChecked(ThemeManager::Mode(action->data().toInt()) == theme.mode());
}

QColor MainWindow::legendTextColor(bool graphVisible) const
{
    const auto &colors = ThemeManager::instance().colors();
    return graphVisible ? colors.text : colors.mutedText;
}

void MainWindow::styleGraph(QCPGraph *graph)
{
    const auto &theme = ThemeManager::instance();
    const bool fast = preferences.lineQuality == AppPreferences::LineQuality::Fast;
    QPen pen(theme.seriesColor(graph->property("colorIndex").toInt()), fast ? 1.0 : 2.0);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(fast ? Qt::BevelJoin : Qt::RoundJoin);
    graph->setPen(pen);
    graph->setScatterStyle(QCPScatterStyle::ssNone);
    graph->setAntialiased(!fast);

    QPen selected(theme.colors().text, 2.0, Qt::DashLine);
    graph->selectionDecorator()->setPen(selected);
}

void MainWindow::applyTheme()
{
    const auto &colors = ThemeManager::instance().colors();
    QCustomPlot *plot = ui->widgetChart;

    plot->setBackground(colors.chartBackground);
    plot->axisRect()->setBackground(colors.chartBackground);

    for (QCPAxis *axis : plot->axisRect()->axes())
    {
        axis->setBasePen(QPen(colors.chartAxis, 1));
        axis->setTickPen(QPen(colors.chartAxis, 1));
        axis->setSubTickPen(QPen(colors.chartAxis, 1));
        axis->setTickLabelColor(colors.mutedText);
        axis->setLabelColor(colors.mutedText);
        axis->grid()->setPen(QPen(colors.chartGrid, 1));
        axis->grid()->setSubGridVisible(false);
        axis->grid()->setZeroLinePen(QPen(colors.chartAxis, 1));
    }

    QColor legendBackground = colors.chartBackground;
    legendBackground.setAlpha(225);
    plot->legend->setBrush(legendBackground);
    plot->legend->setBorderPen(QPen(colors.border));
    plot->legend->setSelectedBorderPen(QPen(colors.accent));
    plot->legend->setTextColor(colors.text);
    plot->legend->setSelectedTextColor(colors.accent);

    for (auto i = 0; i < plot->graphCount(); ++i)
    {
        styleGraph(plot->graph(i));
        if (auto *item = plot->legend->itemWithPlottable(plot->graph(i)))
            item->setTextColor(legendTextColor(plot->graph(i)->visible()));
    }

    if (phaseTracer != nullptr)
    {
        phaseTracer->setPen(QPen(colors.warning, 1.5));
        phaseTracer->setBrush(colors.warning);
    }

    ui->textBrowserLogs->setColors(colors.currentLine, colors.gutter, colors.gutterText);
#ifdef QTSM_HAS_QT_GRAPHS
    if (graphsChartView)
        graphsChartView->applyTheme(preferences.lineQuality == AppPreferences::LineQuality::Fast);
#endif
    on_highlighLog(ui->lineEditHighlight->text());
    plot->replot();
}

void MainWindow::setChartPaused(bool paused)
{
    chartPaused = paused;
    pushButtonPauseChart->setText(paused ? "Resume" : "Pause");

    if (!paused)
        ui->widgetChart->replot(); // auto-track jumps back to live data
}

void MainWindow::scrollChartHistory(int valueMs)
{
    if (updatingHistoryScrollBar)
        return;

    if (!chartPaused)
        pushButtonPauseChart->setChecked(true);

    const double size = ui->widgetChart->xAxis->range().size();
    ui->widgetChart->xAxis->setRange(valueMs / 1000.0, valueMs / 1000.0 + size);
    ui->widgetChart->replot();
}

// Scroll bar covers all recorded data; its page is the visible time window (milliseconds).
void MainWindow::updateHistoryScrollBar()
{
    QCPRange dataRange;
    bool haveData = false;
    for (auto i = 0; i < ui->widgetChart->graphCount(); ++i)
    {
        bool found = false;
        const QCPRange keys = ui->widgetChart->graph(i)->getKeyRange(found);
        if (!found)
            continue;
        dataRange = haveData ? QCPRange(qMin(dataRange.lower, keys.lower), qMax(dataRange.upper, keys.upper)) : keys;
        haveData = true;
    }

    const QCPRange view = ui->widgetChart->xAxis->range();
    updatingHistoryScrollBar = true;
    if (haveData)
    {
        const int minimum = qRound(qMin(dataRange.lower, view.lower) * 1000.0);
        const int maximum = qRound(qMax(dataRange.upper - view.size(), view.lower) * 1000.0);
        scrollBarChartHistory->setRange(minimum, qMax(minimum, maximum));
        scrollBarChartHistory->setPageStep(qMax(1, qRound(view.size() * 1000.0)));
        scrollBarChartHistory->setSingleStep(qMax(1, qRound(view.size() * 100.0)));
        scrollBarChartHistory->setValue(qRound(view.lower * 1000.0));
    }
    else
        scrollBarChartHistory->setRange(0, 0);
    updatingHistoryScrollBar = false;
}

void MainWindow::setupTable()
{
    ui->tableWidgetParsedData->clear();
    ui->tableWidgetParsedData->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    const QStringList parsedDataHeaders = {"Name", "Value", "Min", "Max", "Mean", "Count", "Rate [Hz]"};
    ui->tableWidgetParsedData->setColumnCount(parsedDataHeaders.count());
    for (auto column = 0; column < parsedDataHeaders.count(); ++column)
        ui->tableWidgetParsedData->setHorizontalHeaderItem(column, new QTableWidgetItem(parsedDataHeaders[column]));

    ui->tableWidgetLogTable->setModel(&logTableModel);
    ui->tableWidgetLogTable->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    ui->tableWidgetLogTable->setSortingEnabled(true);
    ui->tableWidgetLogTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed); // no per-row size measuring
    ui->tableWidgetLogTable->verticalHeader()->setDefaultSectionSize(ui->tableWidgetLogTable->fontMetrics().height() + 6);
    connect(&logTableModel, &LogTableModel::columnsAdded, this, [this]()
            {
                if (ui->checkBoxAutoSizeColumnsLogTable->isChecked())
                    ui->tableWidgetLogTable->resizeColumnsToContents();
            });
}

namespace
{
    // Widgets whose state is saved between sessions. Order matters on load:
    // some toggles reset other widgets (e.g. auto logging clears the file name).
    const QStringList persistentWidgets = {
    "checkBoxAppendDate", "checkBoxAutoLogging", "checkBoxAutoReconnect", "checkBoxAutoRefresh",
    "checkBoxAutoRescaleY", "checkBoxAutoSaveBuffer", "checkBoxAutoScrollLogTable",
    "checkBoxAutoSizeColumnsLogTable", "checkBoxAutoTrack", "checkBoxDTR",
    "checkBoxEnableTracer", "checkBoxRAMClearChart", "checkBoxScrollLogEnableSorting",
    "checkBoxScrollToButtom", "checkBoxSendKey", "checkBoxShowControlChars",
    "checkBoxSimplifyLog", "checkBoxTableAutoResize", "checkBoxTruncateFileOnSave",
    "checkBoxWrapText", "comboBoxAddTextMode", "comboBoxBaudRates",
    "comboBoxClockSource", "comboBoxDataBits", "comboBoxExternalTimeFormat",
    "comboBoxFlowControl", "comboBoxGraphDisplayMode", "comboBoxLineEnding",
    "comboBoxLoggingMode", "comboBoxMessagesDisplayMode", "comboBoxParity",
    "comboBoxRAMLoadMode", "comboBoxRAMSaveMode", "comboBoxSerialReadMode",
    "comboBoxStopBits", "comboBoxTextProcessing", "comboBoxTracerStyle",
    "comboBoxUDPReceiveMode", "comboBoxUDPSendMode", "lineEditCustomParsingRules",
    "lineEditExternalClockLabel", "lineEditLoadFilePath", "lineEditSaveFileName",
    "lineEditSaveLogPath", "lineEditUDPTargetIP", "pushButtonEnablePlot",
    "pushButtonEnableTableLog", "spinBoxMaxGraphs", "spinBoxMaxLines",
    "spinBoxMaxRowsLogTable", "spinBoxMaxTimeRange", "spinBoxProcessingDelay", "spinBoxRepeatInterval",
    "spinBoxRemoveOldLabels", "spinBoxScrollingTimeRange", "spinBoxUDPReceivePort",
    "spinBoxUDPTargetPort", "tabWidgetControlSection", "tabWidgetTableView",
    "lineEditOrientationRoll", "lineEditOrientationPitch", "lineEditOrientationYaw", "checkBoxOrientationRadians"};

    QSettings appSettings()
    {
        return QSettings(QSettings::IniFormat, QSettings::UserScope, "QtSerialMonitor", "QtSerialMonitor");
    }

    // Settings from versions <= 1.6 stored sizes as "width height" strings.
    QList<int> legacySizes(const QSettings &settings, const QString &key)
    {
        QList<int> sizes;
        for (const auto &part : settings.value(key).toString().split(' ', Qt::SkipEmptyParts))
            sizes.append(part.toInt());
        return sizes;
    }
}

void MainWindow::settingsLoadAll()
{
    QSettings settings = appSettings();

    if (settings.value("Info/version").toString() != VERSION)
        on_printIntroChangelog();

    // ----- Window and splitter layout ----- //
    if (settings.contains("layout/windowGeometry"))
        restoreGeometry(settings.value("layout/windowGeometry").toByteArray());
    else if (const auto size = legacySizes(settings, "layout/windowSize.size"); size.count() == 2)
        resize(size[0], size[1]);

    for (QSplitter *splitter : {ui->splitterReceivedData, ui->splitterGraphTable})
    {
        const QString key = "layout/" + splitter->objectName();
        if (settings.contains(key + ".state"))
            splitter->restoreState(settings.value(key + ".state").toByteArray());
        else if (const auto sizes = legacySizes(settings, key + ".sizes"); sizes.count() == 2)
            splitter->setSizes(sizes);
    }

    ui->toolButtonAdvancedGraphMenu->setArrowType(ui->splitterGraphTable->sizes().last() > 0 ? Qt::DownArrow : Qt::UpArrow);

    // ----- Send history ----- //
    commandHistory.setItems(settings.value("data/comboBoxSendHistory").toString().split('\n', Qt::SkipEmptyParts));
    ui->comboBoxSend->addItems(commandHistory.items());
    ui->comboBoxSend->lineEdit()->clear();
    macroBar->load(settings);

    // ----- Widgets ----- //
    WidgetSettings::load(settings, this, persistentWidgets);

    if (!ui->lineEditLoadFilePath->text().isEmpty())
    {
        QFile file(ui->lineEditLoadFilePath->text());
        const QList<QTime> timeRange = FileReader::getFileTimeRange(&file);
        ui->timeEditMinParsingTime->setTime(timeRange.first());
        ui->timeEditMaxParsingTime->setTime(timeRange.last());
    }
}

void MainWindow::settingsSaveAll()
{
    QSettings settings = appSettings();

    settings.setValue("Info/version", VERSION);

    settings.setValue("layout/windowGeometry", saveGeometry());
    settings.remove("layout/windowSize.size");
    for (QSplitter *splitter : {ui->splitterReceivedData, ui->splitterGraphTable})
    {
        const QString key = "layout/" + splitter->objectName();
        settings.setValue(key + ".state", splitter->saveState());
        settings.remove(key + ".sizes");
    }

    settings.setValue("data/comboBoxSendHistory", commandHistory.items().join('\n'));
    macroBar->save(settings);

    WidgetSettings::save(settings, this, persistentWidgets);

    settings.sync();
}

namespace
{
    enum ParsedDataColumn { Name, Value, Min, Max, Mean, Count, Rate, ColumnCount };
}

void MainWindow::processTable(QStringList labels, QList<double> values, QList<long> timeStamps)
{
    // Missing-sample counter for each row is kept in the label item's UserRole.
    auto *table = ui->tableWidgetParsedData;
    QHash<QString, int> rowOfLabel;
    for (auto row = 0; row < table->rowCount(); ++row)
        rowOfLabel.insert(table->item(row, Name)->text(), row);

    const auto count = qMin(labels.count(), values.count());
    for (auto i = 0; i < count; ++i)
    {
        int row = rowOfLabel.value(labels[i], -1);
        if (row < 0)
        {
            row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, Name, new QTableWidgetItem(labels[i]));
            for (int column = Value; column < ColumnCount; ++column)
            {
                auto *item = new QTableWidgetItem();
                item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                table->setItem(row, column, item);
            }
            rowOfLabel.insert(labels[i], row);
            parsedDataRowsChanged = true;
        }

        channelStats[labels[i]].add(values[i], i < timeStamps.count() ? timeStamps[i] : 0);
        table->item(row, Name)->setData(Qt::UserRole, 0);
    }

    const int removeThreshold = ui->spinBoxRemoveOldLabels->value();
    if (removeThreshold > 0)
    {
        for (auto row = table->rowCount() - 1; row >= 0; --row) // backwards - rows may be removed
        {
            QTableWidgetItem *labelItem = table->item(row, Name);
            if (labels.contains(labelItem->text()))
                continue;

            const int missing = labelItem->data(Qt::UserRole).toInt() + 1;
            if (missing > removeThreshold)
            {
                channelStats.remove(labelItem->text());
                table->removeRow(row);
                parsedDataRowsChanged = true;
            }
            else
                labelItem->setData(Qt::UserRole, missing);
        }
    }

    // Cell texts and column widths are updated on the refresh tick, not per batch
    // (setting every cell and resizing per batch was ~15% of the GUI thread at 1000 lines/s).
    parsedDataDirty = true;
    requestChartRefresh();
}

void MainWindow::refreshParsedDataTable()
{
    parsedDataDirty = false;
    auto *table = ui->tableWidgetParsedData;

    for (auto row = 0; row < table->rowCount(); ++row)
    {
        const auto it = channelStats.constFind(table->item(row, Name)->text());
        if (it == channelStats.constEnd() || it->count == 0)
            continue;
        table->item(row, Value)->setText(QString::number(it->last));
        table->item(row, Min)->setText(QString::number(it->min));
        table->item(row, Max)->setText(QString::number(it->max));
        table->item(row, Mean)->setText(QString::number(it->mean));
        table->item(row, Count)->setText(QString::number(it->count));
        table->item(row, Rate)->setText(it->rateHz() > 0 ? QString::number(it->rateHz(), 'f', 1) : QString());
    }

    // Resize when rows change, otherwise at most once per second (values change width slowly).
    if (ui->checkBoxTableAutoResize->isChecked() && (parsedDataRowsChanged || !parsedDataResizeClock.isValid() || parsedDataResizeClock.elapsed() > 1000))
    {
        table->resizeColumnsToContents();
        table->resizeRowsToContents();
        parsedDataResizeClock.start();
        parsedDataRowsChanged = false;
    }
}

void MainWindow::clearParsedDataTable()
{
    ui->tableWidgetParsedData->clearContents();
    ui->tableWidgetParsedData->setRowCount(0);
    channelStats.clear();
}

void MainWindow::processLogTable(QList<long> timeTable, QStringList labelTable, QList<double> valueTable)
{
    if (ui->pushButtonEnableTableLog->isChecked()) // checked == logging disabled
        return;

    logTableModel.setMaxRows(ui->spinBoxMaxRowsLogTable->value());
    logTableModel.append(timeTable, labelTable, valueTable);

    // scrollToBottom() re-lays out the whole view (85% of processing time when done per batch),
    // so it happens at most once per chart refresh and only while the table is visible.
    if (ui->checkBoxAutoScrollLogTable->isChecked())
    {
        logTableScrollPending = true;
        requestChartRefresh();
    }
}

void MainWindow::on_printIntroChangelog()
{
    ui->pushButtonTextLogToggle->setChecked(false);
    ui->textBrowserLogs->appendPlainText(INTRO_TEXT);
    ui->textBrowserLogs->appendPlainText("\n" CHANGELOG_TEXT);
}

void MainWindow::on_comboBoxSendReturnPressedSlot()
{
    sendMessageLineEdit();
}

// Sends to every open connection (serial/demo and UDP). Returns false if nothing is connected.
bool MainWindow::sendText(const QString &text)
{
    bool sent = false;

    if (serial.isOpen() || demo.isOpen())
    {
        sendSerial(text);
        sent = true;
    }
    if (networkUDP.isOpen())
    {
        sendUDPDatagram(text);
        sent = true;
    }

    if (!sent)
        addLog("App >>\t Not connected - nothing was sent.", true);
    else if (ui->comboBoxMessagesDisplayMode->currentIndex() == 0)
        addLog("\n >> " + text, ui->comboBoxAddTextMode->currentIndex());

    return sent;
}

void MainWindow::sendMessageLineEdit()
{
    const QString text = ui->comboBoxSend->currentText();
    sendText(text);
    addToCommandHistory(text);
    ui->comboBoxSend->setCurrentText("");
}

void MainWindow::addToCommandHistory(const QString &text)
{
    commandHistory.add(text);
    const QSignalBlocker blocker(ui->comboBoxSend);
    ui->comboBoxSend->clear();
    ui->comboBoxSend->addItems(commandHistory.items()); // most recent first
    ui->comboBoxSend->setCurrentIndex(-1);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // Shell-like Up/Down history in the send box.
    if ((watched == ui->comboBoxSend || watched == ui->comboBoxSend->lineEdit()) && event->type() == QEvent::KeyPress)
    {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        QLineEdit *edit = ui->comboBoxSend->lineEdit();
        if (keyEvent->key() == Qt::Key_Up)
        {
            edit->setText(commandHistory.older(edit->text()));
            return true;
        }
        if (keyEvent->key() == Qt::Key_Down)
        {
            edit->setText(commandHistory.newer(edit->text()));
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::setRepeatSend(bool enabled)
{
    if (!enabled)
    {
        repeatSendTimer.stop();
        return;
    }

    repeatSendText = ui->comboBoxSend->currentText();
    if (!sendText(repeatSendText)) // not connected - don't start
    {
        checkBoxRepeatSend->setChecked(false);
        return;
    }
    addToCommandHistory(repeatSendText);
    ui->comboBoxSend->setCurrentText(repeatSendText);
    repeatSendTimer.start(spinBoxRepeatInterval->value());
}


// Auto-scroll X and rescale Y
void MainWindow::on_chartBeforeReplotSlot()
{
    updateChartRanges();
}

// Auto-scroll X and rescale Y. Runs before every QCustomPlot redraw and on each refresh tick with Qt Graphs.
void MainWindow::updateChartRanges()
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

    if (ui->checkBoxAutoTrack->isChecked() && !chartPaused && ui->widgetChart->graphCount() > 0)
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
                ui->widgetChart->yAxis->scaleRange(1.20); // leave some space at the top and bottom of the chart
            }
        }
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
                menu->addAction("Show exclusively", this, SLOT(on_showSelectedGraphExclusively()));

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
                menu->addAction("Show exclusively", this, SLOT(on_showSelectedGraphExclusively()));

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
            ui->widgetChart->legend->item(i)->setTextColor(legendTextColor(true));
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
        ui->widgetChart->legend->item(i)->setTextColor(legendTextColor(true));
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
            ui->widgetChart->legend->item(i)->setTextColor(legendTextColor(false));
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
            ui->widgetChart->legend->item(i)->setTextColor(legendTextColor(true));
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
            ui->widgetChart->legend->item(i)->setTextColor(legendTextColor(false));
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
    QToolTip::showText(QCursor::pos(),
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

// Starts a background scan; handlePortScan() updates the list when it finishes.
void MainWindow::on_updateSerialDeviceList()
{
    portScanner.scan();
}

void MainWindow::handlePortScan(const QList<PortIdentity> &ports)
{
    knownPorts = ports;
    refreshDeviceList();

    this->radioButtonTimer->start(RADIO_BUTTON_UPDATE_SERIAL_DEVICES_ON_INTERVAL);
    ui->radioButtonDeviceUpdate->setChecked(true);

    if (waitingForReconnect)
        tryReconnect();
}

void MainWindow::refreshDeviceList()
{
    static const QString noDevicesText = "No serial devices detected :(";
    static const QString demoText = "(Demo) Simulated sine waves";

    // Wanted entries as (text, port name); the demo device is listed only while it is in use.
    QList<QPair<QString, QString>> entries;
    for (const auto &device : knownPorts)
        entries.append({"(" + device.portName + ") " + device.description, device.portName});
    if (entries.isEmpty())
        entries.append({noDevicesText, QString()});
    if (demoListed)
        entries.append({demoText, demoPortId});

    bool changed = ui->comboBoxDevices->count() != entries.count();
    for (auto i = 0; !changed && i < entries.count(); ++i)
        changed = ui->comboBoxDevices->itemText(i) != entries[i].first || ui->comboBoxDevices->itemData(i).toString() != entries[i].second;

    if (changed)
    {
        const QString previousPort = ui->comboBoxDevices->currentData().toString();

        ui->comboBoxDevices->clear();
        for (const auto &entry : entries)
            ui->comboBoxDevices->addItem(entry.first, entry.second);

        const int previousIndex = ui->comboBoxDevices->findData(previousPort);
        if (previousIndex >= 0 && !previousPort.isEmpty())
            ui->comboBoxDevices->setCurrentIndex(previousIndex);
    }
}

void MainWindow::on_pushButtonRefresh_clicked()
{
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
    QString output;
    output.reserve(text.size() * 2);

    for (auto i = 0; i < text.size(); ++i)
    {
        const QChar ch = text[i];
        if (ch == '\r')
        {
            output += "\\r";
            if (i + 1 >= text.size() || text[i + 1] != '\n')
                output += '\n'; // lone CR still breaks the line
        }
        else if (ch == '\n')
            output += "\\n\n";
        else if (ch == '\t')
            output += "\\t\t";
        else if (ch == ' ')
            output += QChar(183); // middle dot
        else
            output += ch;
    }

    return output;
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
                bytesText.append(QString::number(static_cast<quint8>(bytes[i]), 2).rightJustified(8, '0') + ' ');
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

void MainWindow::processLogWrite(QString rawLine, QStringList labelList, QList<double> dataList, QList<long> /*timeList*/)
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

void MainWindow::connectTransport(Transport *transport, ReceivePipeline *pipeline)
{
    connect(transport, &Transport::dataReceived, pipeline, &ReceivePipeline::append);

    // Several transports may share one pipeline (serial + demo) - connect its outputs only once,
    // otherwise every line is displayed and parsed once per transport.
    connect(pipeline, &ReceivePipeline::received, this, &MainWindow::displayIncoming, Qt::UniqueConnection);
    connect(pipeline, &ReceivePipeline::linesReceived, this, &MainWindow::processIncomingLines, Qt::UniqueConnection);
}

// Everything received since the last batch - shown as it arrives.
void MainWindow::displayIncoming(const QByteArray &bytes, const QString &text)
{
    QString input = text;

    if (ui->comboBoxTextProcessing->currentIndex() == 1)
        input = input.trimmed();
    else if (ui->comboBoxTextProcessing->currentIndex() == 2)
        input = input.simplified();

    const bool appendAsLine = ui->comboBoxAddTextMode->currentIndex();

    // HEX / BIN show the raw bytes as received, not a UTF-8 round trip.
    if (ui->comboBoxFormat->currentIndex() == 0 && !input.isEmpty())
        addLog(input, appendAsLine);
    else if (ui->comboBoxFormat->currentIndex() == 1)
        addLogBytes(bytes, false, appendAsLine);
    else if (ui->comboBoxFormat->currentIndex() == 2)
        addLogBytes(bytes, true, appendAsLine);
}

// Complete lines only - a value split across two reads is never parsed in halves.
void MainWindow::processIncomingLines(const QString &lines)
{
    if (lines.trimmed().isEmpty())
        return;

    parser.parse(lines, ui->comboBoxClockSource->currentIndex() == 0, ui->comboBoxClockSource->currentIndex() == 1, ui->lineEditExternalClockLabel->text()); // Parse string - split into labels + numeric data
    QStringList labelList = parser.getStringListLabels();
    QList<double> numericDataList = parser.getListNumericValues();
    QList<long> timeStamps = parser.getListTimeStamp();

    this->processChart(labelList, numericDataList, timeStamps);
    this->processLogWrite(lines, labelList, numericDataList, timeStamps);
    this->saveToRAM(labelList, numericDataList, timeStamps, ui->comboBoxRAMSaveMode->currentIndex(), lines);
    this->processTable(labelList, numericDataList, timeStamps);
    this->processLogTable(timeStamps, labelList, numericDataList);
    this->updateOrientation(labelList, numericDataList);
}

void MainWindow::clearGraphData(bool replot)
{
    for (auto i = 0; i < ui->widgetChart->graphCount(); ++i)
    {
        ui->widgetChart->graph(i)->data().data()->clear();
    }

    if (replot)
        ui->widgetChart->replot();
}

void MainWindow::sendUDPDatagram(QString message)
{
    if (!networkUDP.isOpen())
    {
        addLog("App >>\t Unable to send - port closed.", true);
        return;
    }

    if (ui->comboBoxLineEnding->currentIndex() == 1)
    {
        message = message + "\n";
    }
    else if (ui->comboBoxLineEnding->currentIndex() == 2)
    {
        message = message + "\r";
    }
    else if (ui->comboBoxLineEnding->currentIndex() == 3)
    {
        message = message + "\r\n";
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

            // Colours come from the theme palette and don't change when more graphs are added.
            ui->widgetChart->graph()->setProperty("colorIndex", graphColorIndex++);
            styleGraph(ui->widgetChart->graph());

            missingCount.insert(label, 0);
        }
    }

    for (auto i = ui->widgetChart->graphCount() - 1; i >= 0; --i) // backwards - graphs may be removed
    {
        QCPGraph *graph = ui->widgetChart->graph(i);

        for (auto j = 0; j < labelList.count(); ++j)
        {
            if (labelList[j] == graph->name()) // If label matches the graphs name - we can start add points to it
            {
                if (timeStampsList[j] > 0) // 0 = no external clock value was parsed yet
                    graph->addData(timeStampsList[j] / 1000.0, numericDataList[j]);

                // no break - slower but if we keep checking for duplicates, we can add multiple points at once
            }
        }

        if (ui->spinBoxMaxTimeRange->value() > 0)
            graph->data()->removeBefore((timeStampsList.last() / 1000.0) - ui->spinBoxMaxTimeRange->value()); // Remove old points

        if (ui->spinBoxRemoveOldLabels->value() > 0)
        {
            int &missing = missingCount[graph->name()];
            missing = labelList.contains(graph->name()) ? 0 : missing + 1;

            if (missing > ui->spinBoxRemoveOldLabels->value())
            {
                missingCount.remove(graph->name());
                if (phaseTracer != nullptr && phaseTracer->graph() == graph)
                    phaseTracer->setGraph(nullptr);
                ui->widgetChart->removeGraph(graph);
            }
        }
    }

    requestChartRefresh();
}

// New data only schedules a redraw; the chart is redrawn at most chartRefreshRate times per second
// instead of after every batch (redrawing was ~75% of the CPU time at 1000 lines/s).
void MainWindow::requestChartRefresh()
{
    if (!chartRefreshTimer.isActive())
        chartRefreshTimer.start();
}

void MainWindow::refreshChart()
{
    if (orientationDirty && orientationView->isVisible())
        refreshOrientationView();

    if (parsedDataDirty)
        refreshParsedDataTable();

    if (logTableScrollPending && ui->tableWidgetLogTable->isVisible())
    {
        logTableScrollPending = false;
        ui->tableWidgetLogTable->scrollToBottom();
    }

    if (!chartPaused) // keep recording while paused, just don't move the view
    {
#ifdef QTSM_HAS_QT_GRAPHS
        if (graphsChartView && graphsChartView->isVisible())
        {
            updateChartRanges(); // QCustomPlot is hidden and not redrawn, but still holds data and ranges
            graphsChartView->sync(ui->widgetChart);
        }
        else
#endif
            ui->widgetChart->replot();
    }
    updateHistoryScrollBar();
}

void MainWindow::setChartRefreshRate(int framesPerSecond)
{
    chartRefreshTimer.setInterval(1000 / qBound(1, framesPerSecond, 240));
}

void MainWindow::on_setSelectedLabels(QList<QString> *labels, bool customRules)
{

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
    if (phaseTracer != nullptr)
        phaseTracer->setGraph(nullptr); // tracer keeps a raw pointer to its graph

    ui->widgetChart->clearGraphs();
    missingCount.clear();
    graphColorIndex = 0;

    if (replot)
        ui->widgetChart->replot();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (ui->textBrowserLogs->hasFocus())
    {
        if (event->key() == Qt::Key_Alt) // Alt jumps to the newest line
        {
            ui->textBrowserLogs->verticalScrollBar()->setValue(ui->textBrowserLogs->verticalScrollBar()->maximum());
        }
        else if (ui->checkBoxSendKey->isChecked())
        {
            if (!event->text().isEmpty())
                sendText(event->text());
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
        else if (event->key() == Qt::Key_Space || event->key() == Qt::Key_P)
        {
            pushButtonPauseChart->toggle();
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


void MainWindow::sendSerial(QString message)
{
    if (ui->comboBoxLineEnding->currentIndex() == 1)
    {
        message = message + "\n";
    }
    else if (ui->comboBoxLineEnding->currentIndex() == 2)
    {
        message = message + "\r";
    }
    else if (ui->comboBoxLineEnding->currentIndex() == 3)
    {
        message = message + "\r\n";
    }

    const bool sent = demo.isOpen() ? demo.send(message.toUtf8()) : serial.send(message);
    if (!sent)
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
    this->processTable(RAMLabels, RAMData, RAMTime);
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
        colorFormat.setForeground(ThemeManager::instance().colors().text);
        colorFormat.setFontWeight(QFont::Bold);
        colorFormat.setBackground(ThemeManager::instance().colors().searchHighlight);

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
        ui->widgetChart->removeItem(phaseTracer); // plain delete leaves a dangling pointer in QCustomPlot
        phaseTracer = nullptr;
    }

    ui->widgetChart->replot();
}

void MainWindow::createChartTracer()
{
    if (phaseTracer != nullptr)
        ui->widgetChart->removeItem(phaseTracer);

    // add the phase tracer (red circle) which sticks to the graph data:
    this->phaseTracer = new QCPItemTracer(ui->widgetChart);
    this->phaseTracer->setInterpolating(false);

    if (ui->comboBoxTracerStyle->currentText().contains("Crosshair", Qt::CaseSensitivity::CaseInsensitive))
        this->phaseTracer->setStyle(QCPItemTracer::tsCrosshair);
    else
        this->phaseTracer->setStyle(QCPItemTracer::tsCircle);

    this->phaseTracer->setPen(QPen(ThemeManager::instance().colors().warning, 1.5));
    this->phaseTracer->setBrush(ThemeManager::instance().colors().warning);
    this->phaseTracer->setSize(8);
}

void MainWindow::on_pushButtonClearAll_clicked()
{
    ui->widgetChart->xAxis->setRange(-1.0, ui->spinBoxScrollingTimeRange->value());
    ui->widgetChart->yAxis->setRange(-1.0, 1.0);

    parser.restartChartTimer();
    parser.clearExternalClock();

    orientationReceived = false;
    orientationView->setHasData(false);
    clearGraphs(true);

    clearParsedDataTable();
    logTableModel.clear();

    if (ui->checkBoxRAMClearChart->isChecked())
        parser.clearStorage();
}

void MainWindow::on_pushButtonEnablePlot_toggled(bool checked)
{
    ui->pushButtonEnablePlot->setText(checked ? "Resume plotting" : "Stop plotting"); // checked = stopped
}

void MainWindow::on_pushButtonSend_clicked()
{
    sendMessageLineEdit();
}

void MainWindow::on_pushButtonClearHistory_clicked()
{
    commandHistory.clear();
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
    QPalette paletteRed;
    paletteRed.setColor(QPalette::Text, ThemeManager::instance().colors().warning);

    if (arg1 > 10)
        ui->spinBoxMaxGraphs->setPalette(paletteRed);
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
    int newInterval = qMax(1, arg1);

    serialPipeline.setBatchInterval(newInterval);
    udpPipeline.setBatchInterval(newInterval);

    QPalette paletteRed;
    paletteRed.setColor(QPalette::Text, ThemeManager::instance().colors().warning);

    if (newInterval > ui->spinBoxProcessingDelay->minimum())
        ui->spinBoxProcessingDelay->setPalette(paletteRed);
    else
        ui->spinBoxProcessingDelay->setPalette(QPalette());
}

void MainWindow::on_comboBoxTracerStyle_currentTextChanged(const QString &)
{
    if (ui->checkBoxEnableTracer->isChecked())
    {
        if (phaseTracer != nullptr)
        {
            disconnect(ui->widgetChart, SIGNAL(mouseMove(QMouseEvent *)), this, SLOT(on_tracerShowPointValue(QMouseEvent *)));
            ui->widgetChart->removeItem(phaseTracer); // plain delete leaves a dangling pointer in QCustomPlot
            phaseTracer = nullptr;
        }

        createChartTracer();
        connect(ui->widgetChart, SIGNAL(mouseMove(QMouseEvent *)), this, SLOT(on_tracerShowPointValue(QMouseEvent *)));

        ui->widgetChart->replot();
    }
}

bool MainWindow::openSerialPort(const QString &portName)
{
    const bool opened = serial.begin(portName, ui->comboBoxBaudRates->currentText().toInt(),
                                     ui->comboBoxDataBits->currentText(), ui->comboBoxParity->currentText(),
                                     ui->comboBoxStopBits->currentText(), ui->comboBoxFlowControl->currentText(),
                                     ui->checkBoxDTR->isChecked());
    if (!opened)
        return false;

    serialPipeline.reset();
    parser.restartChartTimer();

    if (ui->pushButtonLogging->isChecked() && fileLogger.isOpen() == false) // logger on standby ?
        fileLogger.beginLog(ui->lineEditSaveLogPath->text(), ui->checkBoxAutoLogging->isChecked(), ui->lineEditSaveFileName->text(), ui->checkBoxTruncateFileOnSave->isChecked());

    connectedPort = PortIdentity::fromInfo(QSerialPortInfo(portName));
    addLog("App >>\t Serial port opened. " + serial.getSerialInfo() + " DTR: " + QString::number(ui->checkBoxDTR->isChecked()), true);
    ui->pushButtonSerialConnect->setText("Disconnect");
    return true;
}

void MainWindow::on_pushButtonSerialConnect_toggled(bool checked)
{
    bool inputError = false;

    if (checked)
    {
        QString parsedPortName = ui->comboBoxDevices->currentData().toString();

        if (parsedPortName.isEmpty())
        {
            addLog("App >>\t No devices available", true);
            addLog("App >>\t Unable to open serial port!", true);
            inputError = true;
        }

        if (ui->comboBoxBaudRates->currentText().toInt() <= 0)
        {
            addLog("App >>\t Invalid baud rate!", true);
            inputError = true;
        }

        if (inputError)
        {
            ui->pushButtonSerialConnect->setChecked(false);
            return;
        }

        if (parsedPortName == demoPortId)
        {
            serialPipeline.reset();
            parser.restartChartTimer();
            demo.start(demoRate);

            if (ui->pushButtonLogging->isChecked() && fileLogger.isOpen() == false)
                fileLogger.beginLog(ui->lineEditSaveLogPath->text(), ui->checkBoxAutoLogging->isChecked(), ui->lineEditSaveFileName->text(), ui->checkBoxTruncateFileOnSave->isChecked());

            addLog("App >>\t Demo device started (" + QString::number(demo.rate()) + " samples/s). Anything you send is echoed back.", true);
            ui->pushButtonSerialConnect->setText("Disconnect");
            return;
        }

        if (!openSerialPort(parsedPortName))
        {
            ui->pushButtonSerialConnect->setChecked(false);
            addLog("App >>\t Unable to open serial port!", true);
        }
    }
    else
    {
        if (waitingForReconnect)
        {
            stopReconnecting();
            addLog("App >>\t Reconnecting cancelled.", true);
            ui->pushButtonSerialConnect->setText("Connect");
            return;
        }

        serialPipeline.flush(); // show what was already received

        if (demo.isOpen())
        {
            demo.stop();
            addLog("App >>\t Demo device stopped.", true);
            ui->pushButtonSerialConnect->setText("Connect");
            demoListed = false;
            refreshDeviceList();
        }
        else if (serial.end())
        {
            addLog("App >>\t Connection closed.", true);
            ui->pushButtonSerialConnect->setText("Connect");
        }
        else
        {
            ui->pushButtonSerialConnect->setChecked(true);
            addLog("App >>\t Error - unable to close the connection!", true);
        }

        if (!ui->pushButtonSerialConnect->isChecked() && !ui->pushButtonUDPConnect->isChecked())
        {
            fileLogger.closeFile();
        }
    }
}

void MainWindow::on_actionSave_As_triggered()
{
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
    ui->stackedWidgetGraphView->setCurrentWidget(findChild<QWidget *>("pageOrientation"));
}

void MainWindow::on_printPlot(QPrinter *printer)
{
    printer->setPageSize(QPageSize(QPageSize::A4));
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
#ifdef QTSM_HAS_QT_GRAPHS
    if (graphsChartView)
        graphsChartView->setLegendVisible(checked);
#endif
}

void MainWindow::on_actionSave_graph_as_triggered()
{
    on_actionImage_triggered();
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
            udpPipeline.reset();
            parser.restartChartTimer();

            if (ui->pushButtonLogging->isChecked() && fileLogger.isOpen() == false)
                fileLogger.beginLog(ui->lineEditSaveLogPath->text(), ui->checkBoxAutoLogging->isChecked(), ui->lineEditSaveFileName->text(), ui->checkBoxTruncateFileOnSave->isChecked());


            addLog("App >>\t UDP port opened.", true);

            ui->pushButtonUDPConnect->setText("Close Connection");
        }
        else
        {
            addLog("App >>\t UDP error. Unable to bind", true);
            ui->pushButtonUDPConnect->setChecked(false);
        }
    }
    else
    {
        if (networkUDP.end())
        {
            udpPipeline.flush();

            addLog("App >>\t UDP port closed.", true);


            if (!ui->pushButtonSerialConnect->isChecked() && !ui->pushButtonUDPConnect->isChecked())
                fileLogger.closeFile();

            ui->pushButtonUDPConnect->setText("Open Connection");
        }
    }
}

void MainWindow::on_comboBoxUDPSendMode_currentTextChanged(const QString &arg1)
{
    if (arg1.contains("SpecialAddress") || ui->comboBoxUDPReceiveMode->currentText().contains("SpecialAddress"))
        ui->lineEditUDPTargetIP->setEnabled(true);
    else
        ui->lineEditUDPTargetIP->setEnabled(false);
}

void MainWindow::on_comboBoxUDPReceiveMode_currentTextChanged(const QString &arg1)
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
            if (!fileLogger.beginLog(ui->lineEditSaveLogPath->text(), ui->checkBoxAutoLogging->isChecked(), ui->lineEditSaveFileName->text(), ui->checkBoxTruncateFileOnSave->isChecked()))
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

QString MainWindow::defaultLogPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/QtSerialMonitor/Logs";
}

void MainWindow::on_pushButtonSetDefaultLogPath_clicked()
{
    ui->lineEditSaveLogPath->setText(defaultLogPath());
}

void MainWindow::on_pushButtonAddDateTime_clicked()
{
    ui->lineEditSaveFileName->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh.mm.ss.zzz_") + ui->lineEditSaveFileName->text()); // no ':' - invalid in Windows file names
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
    clearParsedDataTable();
}

void MainWindow::on_tableWidgetParsedData_customContextMenuRequested(const QPoint &pos)
{
    QMenu *menu = new QMenu(this);
    menu->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose);

    menu->addAction("Apply to custom display", this, [this]()
                    { emit on_pushButtonSetSelectedToGraph_clicked(); }); // C++ lambda expression

    menu->addAction("Copy as text", this, [this]()
                    {
                        QString copyText;
                        foreach (auto item, ui->tableWidgetParsedData->selectedItems())
                            copyText.append(item->text() + " ");
                        QApplication::clipboard()->setText(copyText);
                    }); // C++ lambda expression

    menu->addAction("Set as clock label", this, [this]()
                    {
                        const auto selected = ui->tableWidgetParsedData->selectedItems();
                        if (!selected.isEmpty())
                            ui->lineEditExternalClockLabel->setText(selected.first()->text());
                    });

    menu->addAction("Reset statistics", this, [this]()
                    { channelStats.clear(); });

    menu->addAction("Clear table contents", this, [this]()
                    { clearParsedDataTable(); });

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

void MainWindow::on_splitterGraphTable_splitterMoved(int pos, int /*index*/)
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
        ui->pushButtonTextLogToggle->setText("Resume text");
    }
    else
    {
        ui->pushButtonTextLogToggle->setText("Pause text");
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
        ui->lineEditFileInfo->setText("File doesn't exist");
        return;
    }

    QList<QTime> timeRange = FileReader::getFileTimeRange(&file);
    ui->timeEditMinParsingTime->setTime(timeRange.first());
    ui->timeEditMaxParsingTime->setTime(timeRange.last());

    QString info = "Size: " + QString::number(file.size());
    ui->lineEditFileInfo->setText(info);
}

void MainWindow::handleFileLoaded(const FileReader::Result &result)
{
    ui->pushButtonLoadFile->setText("Load File");

    if (!result.error.isEmpty())
    {
        addLog("App >>\t Error - unable to read file: " + result.error, true);
        ui->progressBarLoadFile->setValue(0);
        return;
    }

    if (result.cancelled)
    {
        addLog("App >>\t File loading cancelled.", true);
        ui->progressBarLoadFile->setValue(0);
        return;
    }

    parser.clearStorage();
    this->processChart(result.labels, result.values, result.timeStamps);
    this->processTable(result.labels, result.values, result.timeStamps);
    this->saveToRAM(result.labels, result.values, result.timeStamps);

    if (ui->checkBoxAppendLoadedTextToLog->isChecked())
        ui->textBrowserLogs->appendPlainText(result.text);

    ui->progressBarLoadFile->setValue(100);
    addLog("App >>\t Loaded " + QString::number(result.values.count()) + " values from file.", true);
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
    if (fileReader.isBusy())
    {
        fileReader.cancel(); // handleFileLoaded() resets the UI
        return;
    }

    ui->progressBarLoadFile->setValue(0);

    const QString path = ui->lineEditLoadFilePath->text().trimmed();
    const QFileInfo fileInfo(path);
    if (!fileInfo.isFile() || !fileInfo.isReadable())
    {
        addLog("App >>\t Error - invalid file path !", true);
        return;
    }

    FileReader::Options options;
    options.csv = ui->comboBoxLogFormat->currentIndex() == 0;
    options.useExternalClockLabel = ui->comboBoxClockSource->currentIndex() == 1;
    options.clockLabel = ui->lineEditExternalClockLabel->text();
    options.minTime = ui->timeEditMinParsingTime->time();
    options.maxTime = ui->timeEditMaxParsingTime->time();

    this->clearGraphs(true);

    if (fileReader.load(path, options))
        ui->pushButtonLoadFile->setText("Cancel");
}

void MainWindow::on_lineEditLoadFilePath_customContextMenuRequested(const QPoint &pos)
{
    QMenu *menu = new QMenu(this);
    menu->setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose);

    menu->addAction("Open file", this, [this]()
                    { QDesktopServices::openUrl(QUrl::fromLocalFile(ui->lineEditLoadFilePath->text())); }); // C++ lambda expression

    menu->addAction("Open folder", this, [this]()
                    { QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(ui->lineEditLoadFilePath->text()).absolutePath())); }); // C++ lambda expression

    menu->popup(ui->lineEditLoadFilePath->mapToGlobal(pos));
}

// "Save" on the Parser memory tab (enabled when automatic saving is off): stores the plotted data.
void MainWindow::on_pushButtonSaveParserMemory_clicked()
{
    QStringList labels;
    QList<double> values;
    QList<long> timeStamps;

    for (auto i = 0; i < ui->widgetChart->graphCount(); ++i)
    {
        const QCPGraph *graph = ui->widgetChart->graph(i);
        for (auto it = graph->data()->constBegin(); it != graph->data()->constEnd(); ++it)
        {
            labels.append(graph->name()); // one label per value - the lists are parallel
            values.append(it->value);
            timeStamps.append(qRound64(it->key * 1000.0)); // chart keys are seconds, storage is milliseconds
        }
    }

    const QString text = ui->comboBoxRAMSaveMode->currentIndex() == 1 ? ui->textBrowserLogs->toPlainText() : QString();
    parser.appendSetToMemory(labels, values, timeStamps, text);
    addLog("App >>\t Saved " + QString::number(values.count()) + " values to parser memory.", true);
}

void MainWindow::on_actionInfo_triggered()
{
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

void MainWindow::on_actionHide_parser_data_triggered()
{
    ui->splitterGraphTable->setSizes({ui->splitterGraphTable->width(), 0});
}

void MainWindow::on_actionShow_parser_data_triggered()
{
    ui->splitterGraphTable->setSizes({ui->splitterGraphTable->width(), ui->splitterGraphTable->height() / 2});
}

void MainWindow::on_comboBoxLogFormat_currentIndexChanged(int index)
{
    ui->comboBoxLoggingMode->setEnabled((bool)index);
    ui->checkBoxSimplifyLog->setEnabled((bool)index);

    if (ui->comboBoxLogFormat->currentText().contains("txt"))
        ui->lineEditSaveFileName->setText(ui->lineEditSaveFileName->text().replace("csv", "txt"));
    else if (ui->comboBoxLogFormat->currentText().contains("csv"))
        ui->lineEditSaveFileName->setText(ui->lineEditSaveFileName->text().replace("txt", "csv"));
}

void MainWindow::on_comboBoxGraphDisplayMode_currentIndexChanged(int index)
{
    clearGraphs(true);
    loadFromRAM(0);

    QPalette paletteRed;
    paletteRed.setColor(QPalette::Text, ThemeManager::instance().colors().warning);

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
        ui->comboBoxGraphDisplayMode->setPalette(paletteRed);
    }
}

void MainWindow::on_pushButtonClearLogTable_clicked()
{
    logTableModel.clear();
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

void MainWindow::on_lineEditLoadFilePath_textChanged(const QString &/*arg1*/)
{
    QFile file(ui->lineEditLoadFilePath->text());

    if (!file.exists())
    {
        ui->lineEditFileInfo->setText("File doesn't exist");
        return;
    }

    QList<QTime> timeRange = FileReader::getFileTimeRange(&file);
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
    QString filename = QFileDialog::getSaveFileName(this, tr("Save File"), QDir::homePath() + "/export.png",
                                                    tr("PNG image (*.png);;JPEG image (*.jpg *.jpeg);;BMP image (*.bmp);;PDF vector (*.pdf)"));
    if (filename.isEmpty())
        return;

    const QString suffix = QFileInfo(filename).suffix().toLower();
    bool saved = false;

    if (suffix == "jpg" || suffix == "jpeg")
        saved = ui->widgetChart->saveJpg(filename);
    else if (suffix == "bmp")
        saved = ui->widgetChart->saveBmp(filename);
    else if (suffix == "pdf")
        saved = ui->widgetChart->savePdf(filename);
    else
    {
        if (suffix != "png")
            filename += ".png";
        saved = ui->widgetChart->savePng(filename);
    }

    addLog(saved ? "App >>\t Graph saved to " + filename : "App >>\t Error - unable to save graph to " + filename, true);
}

void MainWindow::on_actionto_csv_triggered()
{
    QStringList columnNames;
    QList<QList<double>> columnsData;

    parser.getCSVReadyData(&columnNames, &columnsData);

    this->exportArraysToCSV(columnNames, columnsData, ',');
}
