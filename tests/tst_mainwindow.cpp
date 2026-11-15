// Integration test: drives the real MainWindow headlessly and feeds it data over UDP.
#include "mainwindow.h"
#include "orientationview.h"
#ifdef QTSM_HAS_QT_GRAPHS
#include "graphschartview.h"
#include <QValueAxis>
#include <QQuickItem>
#include <QMouseEvent>
#include <QWheelEvent>
#endif
#include "theme.h"
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProgressDialog>
#include <QPushButton>
#include <QTextBrowser>
#include <QScrollBar>
#include <QSpinBox>
#include <QToolButton>
#include <QTableWidget>
#include <QTabWidget>

#include <QSettings>
#include <QTemporaryDir>
#include <QUdpSocket>
#include <QtTest>

class TestMainWindow : public QObject
{
    Q_OBJECT

    QTemporaryDir settingsDir;
    MainWindow *window = nullptr;
    QUdpSocket sender;
    quint16 port = 0;

    template <typename T>
    T *find(const char *name)
    {
        T *widget = window->findChild<T *>(name);
        if (!widget)
            qFatal("widget %s not found", name);
        return widget;
    }

    void send(const QByteArray &datagram)
    {
        sender.writeDatagram(datagram, QHostAddress::LocalHost, port);
    }

    QString logText() { return find<QPlainTextEdit>("textBrowserLogs")->toPlainText(); }

    QAbstractItemModel *logTable() { return find<QTableView>("tableWidgetLogTable")->model(); }
    QString logCell(int row, int column) { return logTable()->index(row, column).data().toString(); }
    QString logHeader(int column) { return logTable()->headerData(column, Qt::Horizontal).toString(); }
    void showSamples() { find<QTabWidget>("tabWidgetTableView")->setCurrentWidget(find<QWidget>("tabWidgetTableViewPage2")); }

    QStringList graphNames()
    {
        QStringList names;
        auto *chart = find<QCustomPlot>("widgetChart");
        for (auto i = 0; i < chart->graphCount(); ++i)
            names.append(chart->graph(i)->name());
        return names;
    }

private slots:
    void initTestCase()
    {
        // Keep the user's real settings untouched.
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir.path());
    }

    void init()
    {
        window = new MainWindow;
        window->show();

        QUdpSocket probe; // find a free port
        QVERIFY(probe.bind(QHostAddress(QHostAddress::LocalHost), quint16(0)));
        port = probe.localPort();
        probe.close();

        find<QSpinBox>("spinBoxUDPReceivePort")->setValue(port);
        find<QComboBox>("comboBoxUDPReceiveMode")->setCurrentText("LocalHost");
        find<QCheckBox>("checkBoxShowTime")->setChecked(false);
        find<QCheckBox>("checkBoxShowControlChars")->setChecked(false);
        find<QComboBox>("comboBoxTextProcessing")->setCurrentIndex(0);
        find<QComboBox>("comboBoxAddTextMode")->setCurrentIndex(0);
        find<QComboBox>("comboBoxFormat")->setCurrentIndex(0);
        find<QPushButton>("pushButtonEnablePlot")->setChecked(false);      // checked == disabled
        find<QPushButton>("pushButtonUDPConnect")->setChecked(true);
        QVERIFY(find<QPushButton>("pushButtonUDPConnect")->isChecked());
        find<QPlainTextEdit>("textBrowserLogs")->clear();
    }

    void cleanup()
    {
        find<QPushButton>("pushButtonUDPConnect")->setChecked(false);
        delete window;
        window = nullptr;
    }

    void plotsAndTablesFromUdp()
    {
        find<QSpinBox>("spinBoxRemoveOldLabels")->setValue(3);

        // "Max missing" counts processing batches, so give each line its own batch.
        for (const char *line : {"a 1 b 2 c 3", "a 4 b 5", "b 6", "b 7", "b 8", "b 9 d +1e2"})
        {
            send(line);
            QTest::qWait(30);
        }

        // a and c disappear after more than 3 updates without data
        QTRY_COMPARE(graphNames(), (QStringList{"b", "d"}));

        auto *table = find<QTableWidget>("tableWidgetParsedData");
        QTRY_COMPARE(table->rowCount(), 2);
        QTRY_COMPARE(table->item(0, 1)->text(), QString("9")); // cells refresh on the chart tick
        QTRY_COMPARE(table->item(1, 1)->text(), QString("100"));

        // Statistics columns for "b" (values 2, 5, 6, 7, 8, 9)
        QCOMPARE(table->horizontalHeaderItem(2)->text(), QString("Min"));
        QCOMPARE(table->item(0, 2)->text(), QString("2"));
        QCOMPARE(table->item(0, 3)->text(), QString("9"));
        QCOMPARE(table->item(0, 4)->text(), QString::number(37.0 / 6.0));
        QCOMPARE(table->item(0, 5)->text(), QString("6"));

        // Samples show what the chart holds: a and c were removed with their graphs.
        showSamples();
        QTRY_COMPARE(logTable()->rowCount(), 6);
        QCOMPARE(logTable()->columnCount(), 3);
        QCOMPARE(logHeader(1), QString("b"));
        QCOMPARE(logHeader(2), QString("d"));
        QCOMPARE(logCell(0, 1), QString("2"));
        QCOMPARE(logCell(5, 1), QString("9"));
        QCOMPARE(logCell(5, 2), QString("100")); // same line, same row
        QCOMPARE(logCell(4, 2), QString());
    }

    void logTableKeepsLinesFromOneBatch()
    {
        find<QSpinBox>("spinBoxProcessingDelay")->setValue(200); // force both lines into one batch
        showSamples();
        send("x 1 y 2");
        send("x 3 y 4");

        QTRY_COMPARE(logTable()->rowCount(), 2);
        QCOMPARE(logCell(0, 1), QString("1"));
        QCOMPARE(logCell(1, 1), QString("3"));
        QCOMPARE(logCell(1, 2), QString("4"));
        find<QSpinBox>("spinBoxProcessingDelay")->setValue(1);
    }

    void hexViewShowsRawBytes()
    {
        find<QComboBox>("comboBoxFormat")->setCurrentIndex(1);
        send(QByteArray("A\0\xff", 3));
        QTRY_COMPARE(logText(), QString("41 00 ff"));
    }

    void binaryViewIsUnsigned()
    {
        find<QComboBox>("comboBoxFormat")->setCurrentIndex(2);
        send(QByteArray("\x80\x01", 2));
        QTRY_COMPARE(logText().trimmed(), QString("10000000 00000001"));
    }

    void utf8SplitAcrossDatagrams()
    {
        const QByteArray text = QString::fromUtf8("zażółć").toUtf8();
        send(text.left(3));
        QTest::qWait(30);
        send(text.mid(3));
        QTRY_COMPARE(logText(), QString::fromUtf8("zażółć"));
    }

    void controlCharactersVisible()
    {
        find<QCheckBox>("checkBoxShowControlChars")->setChecked(true);
        send("a b\tc\r\nd\re\n");
        QTRY_COMPARE(logText(), QString::fromUtf8("a·b\\t\tc\\r\\n\nd\\r\ne\\n\n"));
    }

    void samplesFollowTheChart()
    {
        showSamples();
        send("s 1");
        QTest::qWait(30);
        send("s 2");
        QTRY_COMPARE(logTable()->rowCount(), 2);

        // Clear plot (no new data afterwards) empties the table too.
        find<QPushButton>("pushButtonPauseChart")->setChecked(true);
        find<QPushButton>("pushButtonClearGraphs")->click();
        QCOMPARE(logTable()->rowCount(), 0);
        find<QPushButton>("pushButtonPauseChart")->setChecked(false);

        // Rows the chart drops ("Keep data for") leave the table.
        auto *chart = find<QCustomPlot>("widgetChart");
        send("s 3");
        QTRY_COMPARE(logTable()->rowCount(), 1);
        chart->graph(0)->addData(chart->graph(0)->dataMainKey(0) + 10, 4);
        chart->graph(0)->data()->removeBefore(chart->graph(0)->dataMainKey(0) + 5);
        chart->replot();
        QCOMPARE(logTable()->rowCount(), 1);
        QCOMPARE(logCell(0, 1), QString("4"));

        // Hidden table: caught up when shown again.
        find<QTabWidget>("tabWidgetTableView")->setCurrentIndex(0);
        send("s 5");
        QTRY_COMPARE(chart->graph(0)->dataCount(), 2);
        showSamples();
        QCOMPARE(logTable()->rowCount(), 2);
    }

    void samplesLinkedWithChart()
    {
        showSamples();
        for (const char *line : {"k 1", "k 2", "k 3", "k 4"})
        {
            send(line);
            QTest::qWait(30);
        }
        QTRY_COMPARE(logTable()->rowCount(), 4);
        auto *chart = find<QCustomPlot>("widgetChart");
        auto *view = find<QTableView>("tableWidgetLogTable");
        const double key1 = chart->graph(0)->dataMainKey(1);

        // Current row -> marker on the chart.
        view->setCurrentIndex(logTable()->index(1, 1));
        auto *marker = chart->findChild<QCPItemStraightLine *>("sampleMarker");
        QVERIFY(marker && marker->visible());
        QCOMPARE(marker->point1->key(), key1);

        // Double-click -> the chart shows that time and stops following live data.
        find<QCheckBox>("checkBoxAutoTrack")->setChecked(true);
        const double width = chart->xAxis->range().size();
        const QRect cell = view->visualRect(logTable()->index(1, 1));
        QTest::mouseClick(view->viewport(), Qt::LeftButton, {}, cell.center()); // a double-click starts with a click
        QTest::mouseDClick(view->viewport(), Qt::LeftButton, {}, cell.center());
        QVERIFY(!find<QCheckBox>("checkBoxAutoTrack")->isChecked());
        QVERIFY(qAbs(chart->xAxis->range().center() - key1) < 1e-9);
        QVERIFY(qAbs(chart->xAxis->range().size() - width) < 1e-9);

        // "Go to chart view" selects the first row of what the chart shows.
        const double key2 = chart->graph(0)->dataMainKey(2);
        chart->xAxis->setRange(key2 - 0.001, key2 + 10);
        find<QPushButton>("pushButtonSamplesGoToChart")->click();
        QCOMPARE(view->currentIndex().row(), 2);
        QVERIFY(!find<QCheckBox>("checkBoxAutoScrollLogTable")->isChecked());
        QCOMPARE(marker->point1->key(), key2);
        find<QCheckBox>("checkBoxAutoScrollLogTable")->setChecked(true);
        find<QCheckBox>("checkBoxAutoTrack")->setChecked(true);

        // Clearing the chart removes the marker.
        find<QPushButton>("pushButtonClearGraphs")->click();
        QTRY_VERIFY(!marker->visible());
    }

    void settingsFromOlderVersionLoad()
    {
        {
            QSettings legacy(QSettings::IniFormat, QSettings::UserScope, "QtSerialMonitor", "QtSerialMonitor");
            legacy.clear();
            legacy.setValue("Info/version", "1.53");
            legacy.setValue("layout/windowSize.size", "1111 777");
            legacy.setValue("GUI_Elements/checkBoxDTR.isChecked", true);
            legacy.setValue("GUI_Elements/comboBoxParity.currentIndex", 2);
            legacy.setValue("GUI_Elements/comboBoxBaudRates.currentIndex", 9999); // out of range - must be ignored
            legacy.setValue("layout/spinBoxMaxLines.value", 1234);
            legacy.setValue("data/lineEditUDPTargetIP.text", "10.0.0.7");
            legacy.setValue("data/comboBoxSendHistory", "AT\nAT+RST\n");
        }

        MainWindow restored;
        auto findIn = [&](const char *name) { return restored.findChild<QWidget *>(name); };

        QCOMPARE(restored.size(), QSize(1111, 777));
        QVERIFY(qobject_cast<QCheckBox *>(findIn("checkBoxDTR"))->isChecked());
        QCOMPARE(qobject_cast<QComboBox *>(findIn("comboBoxParity"))->currentIndex(), 2);
        QVERIFY(qobject_cast<QComboBox *>(findIn("comboBoxBaudRates"))->currentIndex() < 9999);
        QCOMPARE(qobject_cast<QSpinBox *>(findIn("spinBoxMaxLines"))->value(), 1234);
        QCOMPARE(qobject_cast<QLineEdit *>(findIn("lineEditUDPTargetIP"))->text(), QString("10.0.0.7"));
        QCOMPARE(qobject_cast<QComboBox *>(findIn("comboBoxSend"))->count(), 2);

        // Save in the new format, then load it again.
        qobject_cast<QSpinBox *>(findIn("spinBoxMaxLines"))->setValue(4321);
        QVERIFY(QMetaObject::invokeMethod(&restored, "on_aboutToQuitSlot"));

        QSettings saved(QSettings::IniFormat, QSettings::UserScope, "QtSerialMonitor", "QtSerialMonitor");
        QVERIFY(saved.contains("layout/windowGeometry"));
        QVERIFY(!saved.contains("layout/windowSize.size"));
        QCOMPARE(saved.value("layout/spinBoxMaxLines.value").toInt(), 4321);
        QCOMPARE(saved.value("Info/version").toString(), QString(VERSION));

        MainWindow reloaded;
        QCOMPARE(reloaded.findChild<QSpinBox *>("spinBoxMaxLines")->value(), 4321);
        QCOMPARE(reloaded.findChild<QComboBox *>("comboBoxSend")->count(), 2);
        saved.clear();
    }

    void demoDeviceStreamsSines()
    {
        find<QPushButton>("pushButtonUDPConnect")->setChecked(false);
        window->startDemo(100);
        QVERIFY2(find<QPushButton>("pushButtonSerialConnect")->isChecked(), qPrintable(logText()));

        QTRY_COMPARE(graphNames(), (QStringList{"sin1", "sin2", "noisy"}));
        QTRY_VERIFY(find<QCustomPlot>("widgetChart")->graph(0)->dataCount() > 10);

        // Each line is processed exactly once: consecutive samples must differ in time by ~10 ms (100 Hz),
        // not come in pairs a few ms apart.
        QCPGraph *sine = find<QCustomPlot>("widgetChart")->graph(0);
        QTRY_VERIFY(sine->dataCount() > 20);
        const double span = sine->dataMainKey(sine->dataCount() - 1) - sine->dataMainKey(0);
        const double perSecond = (sine->dataCount() - 1) / span;
        QVERIFY2(perSecond < 150, qPrintable(QString("%1 samples/s for a 100 Hz device").arg(perSecond)));

        // Sent text is echoed back by the demo device.
        auto *send = find<QComboBox>("comboBoxSend");
        send->setCurrentText("hello demo");
        find<QPushButton>("pushButtonSend")->click();
        QTRY_VERIFY(logText().contains("demo> received: hello demo"));

        find<QPushButton>("pushButtonSerialConnect")->setChecked(false);
        QVERIFY(!find<QPushButton>("pushButtonSerialConnect")->isChecked());

        // Hidden from the device list unless running; Tools -> Demo device starts and stops it.
        auto *devices = find<QComboBox>("comboBoxDevices");
        QCOMPARE(devices->findData("demo:"), -1);
        auto *demoAction = window->findChild<QAction *>("actionDemoDevice");
        QVERIFY(demoAction);
        demoAction->trigger();
        QVERIFY(demoAction->isChecked());
        QVERIFY(find<QPushButton>("pushButtonSerialConnect")->isChecked());
        QVERIFY(devices->findData("demo:") >= 0);
        demoAction->trigger();
        QVERIFY(!demoAction->isChecked());
        QVERIFY(!find<QPushButton>("pushButtonSerialConnect")->isChecked());
        QCOMPARE(devices->findData("demo:"), -1);
    }

    void pauseKeepsRecordingAndScrollsHistory()
    {
        auto *chart = find<QCustomPlot>("widgetChart");
        auto *pause = find<QPushButton>("pushButtonPauseChart");
        auto *history = find<QScrollBar>("scrollBarChartHistory");

        find<QPushButton>("pushButtonUDPConnect")->setChecked(false);
        find<QSpinBox>("spinBoxScrollingTimeRange")->setValue(1); // view the last 1 s
        window->startDemo(200);
        // Record more history than fits in the view.
        QTRY_VERIFY_WITH_TIMEOUT(chart->graphCount() == 3 && chart->graph(0)->dataCount() > 1
                                     && chart->graph(0)->dataMainKey(chart->graph(0)->dataCount() - 1) - chart->graph(0)->dataMainKey(0) > 2.0,
                                 10000);

        pause->setChecked(true);
        const QCPRange frozen = chart->xAxis->range();
        const int countAtPause = chart->graph(0)->dataCount();
        QTest::qWait(300);
        QVERIFY(chart->graph(0)->dataCount() > countAtPause); // still recording
        QCOMPARE(chart->xAxis->range().lower, frozen.lower);   // view does not move

        // Scrolling to the oldest data moves the view back.
        history->setValue(history->minimum());
        QVERIFY(chart->xAxis->range().lower < frozen.lower);

        // Resume returns to live data and dragging the scroll bar pauses again.
        pause->setChecked(false);
        QTRY_VERIFY(chart->xAxis->range().upper >= chart->graph(0)->dataMainKey(chart->graph(0)->dataCount() - 1));
        history->setValue(history->minimum());
        QVERIFY(pause->isChecked());

        find<QPushButton>("pushButtonSerialConnect")->setChecked(false);
    }

    void commandHistoryMacrosAndRepeat()
    {
        find<QPushButton>("pushButtonUDPConnect")->setChecked(false);
        window->startDemo(1);
        auto *send = find<QComboBox>("comboBoxSend");
        QLineEdit *edit = send->lineEdit();

        for (const char *command : {"first", "second", "first"})
        {
            send->setCurrentText(command);
            find<QPushButton>("pushButtonSend")->click();
        }
        QCOMPARE(send->count(), 2);
        QCOMPARE(send->itemText(0), QString("first")); // most recent first, no duplicates

        // Keys go to the combo box, as with real keyboard focus (the line edit's focus proxy).
        edit->setText("draft");
        QTest::keyClick(send, Qt::Key_Up);
        QCOMPARE(edit->text(), QString("first")); // Up = most recent, like a terminal
        QTest::keyClick(send, Qt::Key_Up);
        QCOMPARE(edit->text(), QString("second"));
        QTest::keyClick(send, Qt::Key_Down);
        QCOMPARE(edit->text(), QString("first"));
        QTest::keyClick(send, Qt::Key_Down);
        QCOMPARE(edit->text(), QString("draft"));

        // Same through the line edit itself.
        QTest::keyClick(edit, Qt::Key_Up);
        QCOMPARE(edit->text(), QString("first"));
        QTest::keyClick(edit, Qt::Key_Down);
        QCOMPARE(edit->text(), QString("draft"));

        // Macro button sends its text.
        auto *macros = find<MacroBar>("macroBar");
        macros->setMacros({{"Reset", "AT+RST"}});
        find<QToolButton>("toolButtonMacro0")->click();
        QTRY_VERIFY(logText().contains("demo> received: AT+RST"));

        // Periodic send repeats until unchecked.
        send->setCurrentText("tick");
        find<QSpinBox>("spinBoxRepeatInterval")->setValue(20);
        find<QCheckBox>("checkBoxRepeatSend")->setChecked(true);
        QTRY_VERIFY(logText().count("demo> received: tick") >= 3);
        find<QCheckBox>("checkBoxRepeatSend")->setChecked(false);
        QTest::qWait(100); // let echoes already in the receive pipeline reach the log
        const int sent = logText().count("demo> received: tick");
        QTest::qWait(150);
        QCOMPARE(logText().count("demo> received: tick"), sent);

        // Repeat stops by itself when the connection closes.
        find<QCheckBox>("checkBoxRepeatSend")->setChecked(true);
        find<QPushButton>("pushButtonSerialConnect")->setChecked(false);
        QTRY_VERIFY(!find<QCheckBox>("checkBoxRepeatSend")->isChecked());
    }

    void themeSwitching()
    {
        auto &theme = ThemeManager::instance();
        auto *chart = find<QCustomPlot>("widgetChart");

        theme.apply(ThemeManager::Mode::Dark);
        QVERIFY(QApplication::palette().color(QPalette::Window).lightness() < 100);
        QVERIFY(chart->axisRect()->backgroundBrush().color().lightness() < 100);
        QCOMPARE(find<QToolButton>("toolButtonTheme")->text(), QString("Dark"));

        theme.apply(ThemeManager::Mode::Light);
        QVERIFY(QApplication::palette().color(QPalette::Window).lightness() > 200);
        QVERIFY(chart->axisRect()->backgroundBrush().color().lightness() > 200);

        // Switching to a longer label must not truncate the corner button ("S...m").
        theme.apply(ThemeManager::Mode::Dark);
        theme.apply(ThemeManager::Mode::System);
        auto *button = find<QToolButton>("toolButtonTheme");
        QCOMPARE(button->text(), QString("System"));
        QVERIFY(button->width() >= button->sizeHint().width());
        theme.apply(ThemeManager::Mode::Light);

        theme.save();
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, "QtSerialMonitor", "QtSerialMonitor");
        QCOMPARE(settings.value("GUI_Elements/theme").toString(), QString("light"));
        settings.remove("GUI_Elements/theme");
        theme.apply(ThemeManager::Mode::System);
    }

    void performanceSettings()
    {
        send("p 1 q 2");
        QTRY_COMPARE(graphNames().count(), 2);
        QCPGraph *graph = find<QCustomPlot>("widgetChart")->graph(0);
        QVERIFY(graph->antialiased());

        AppPreferences fast;
        fast.chartRefreshRate = 60;
        fast.lineQuality = AppPreferences::LineQuality::Fast;
        window->applyPreferences(fast);
        QVERIFY(!graph->antialiased());
        QCOMPARE(graph->pen().widthF(), 1.0);
        QCOMPARE(AppPreferences::load().chartRefreshRate, 60);
        QVERIFY(AppPreferences::load().lineQuality == AppPreferences::LineQuality::Fast);

        // OpenGL is optional: either it starts, or the app says it fell back (e.g. CI without a GPU).
        if (AppPreferences::openGlAvailable())
        {
            AppPreferences gl;
            gl.openGl = true;
            window->applyPreferences(gl);
            auto *chart = find<QCustomPlot>("widgetChart");
            QVERIFY(chart->openGl() || logText().contains("OpenGL could not be started"));
            QVERIFY(AppPreferences::load().openGl);
            chart->replot();
        }

        window->applyPreferences(AppPreferences()); // back to defaults
        QVERIFY(!find<QCustomPlot>("widgetChart")->openGl());
        QVERIFY(graph->antialiased());
        QCOMPARE(AppPreferences::load().chartRefreshRate, 30);
    }

    void orientationViewWithDemo()
    {
        find<QPushButton>("pushButtonUDPConnect")->setChecked(false);
        auto *view = find<OrientationView>("orientationView");

        // Plain chart: the demo sends no orientation.
        window->startDemo(100);
        QTRY_VERIFY(graphNames().contains("sin1"));
        QVERIFY(!graphNames().contains("roll"));

        // 3D view shown: the demo adds roll/pitch/yaw and the board follows them.
        window->findChild<QAction *>("action3D_orientation")->trigger();
        QVERIFY(view->isVisible());
        QTRY_VERIFY(graphNames().contains("roll"));
        QTRY_VERIFY(view->roll() != 0.0 || view->pitch() != 0.0);
        const double yawBefore = view->yaw();
        QTRY_VERIFY(view->yaw() != yawBefore); // keeps moving

        find<QPushButton>("pushButtonOrientationBack")->click();
        QVERIFY(!view->isVisible());
        find<QPushButton>("pushButtonSerialConnect")->setChecked(false);
    }

    void saveChartToParserMemory()
    {
        find<QCheckBox>("checkBoxAutoSaveBuffer")->setChecked(false); // enables the Save button
        auto *save = find<QPushButton>("pushButtonSaveParserMemory");
        QVERIFY(save->isEnabled());

        send("m 1 n 2");
        QTest::qWait(30);
        send("m 3 n 4");
        QTRY_VERIFY(graphNames().contains("m"));
        QTRY_COMPARE(find<QCustomPlot>("widgetChart")->graph(0)->dataCount(), 2);

        save->click();
        QTRY_VERIFY(logText().contains("Saved 4 values to parser memory"));

        // Reload from memory into a cleared chart.
        find<QCheckBox>("checkBoxRAMClearChart")->setChecked(true);
        find<QPushButton>("pushButtonClearGraphs")->click();
        find<QPushButton>("pushButtonLoadRAMBuffer")->click();
        QTRY_COMPARE(find<QCustomPlot>("widgetChart")->graph(0)->dataCount(), 2);
        find<QCheckBox>("checkBoxAutoSaveBuffer")->setChecked(true);
    }

    void qtGraphsChartEngine()
    {
#ifdef QTSM_HAS_QT_GRAPHS
        find<QPushButton>("pushButtonUDPConnect")->setChecked(false);
        AppPreferences graphs;
        graphs.chartEngine = AppPreferences::ChartEngine::QtGraphs;
        window->applyPreferences(graphs);

        auto *view = window->findChild<GraphsChartView *>("graphsChartView");
        if (!view)
        {
            QVERIFY(logText().contains("Qt Graphs could not be started")); // e.g. no GPU on CI
            window->applyPreferences(AppPreferences());
            return;
        }
        QVERIFY(view->isVisible());
        QVERIFY(!find<QCustomPlot>("widgetChart")->isVisible());

        window->startDemo(100);
        QTRY_COMPARE(view->seriesCount(), 3);
        QTRY_VERIFY(view->pointCount("sin1") > 10);

        // The time axis follows the live data like the QCustomPlot chart.
        auto *axis = view->rootObject()->findChild<QValueAxis *>("axisX");
        const double before = axis->max();
        QTRY_VERIFY(axis->max() > before);

        // Mouse interactions change the chart ranges, like on the QCustomPlot chart.
        find<QCheckBox>("checkBoxAutoTrack")->setChecked(false);
        auto *chart = find<QCustomPlot>("widgetChart");
        const QPoint centre(view->width() / 2, view->height() / 2);
        auto mouse = [view](QEvent::Type type, QPoint pos, Qt::MouseButton button, Qt::MouseButtons buttons)
        {
            QMouseEvent event(type, QPointF(pos), QPointF(view->mapToGlobal(pos)), button, buttons, Qt::NoModifier);
            QApplication::sendEvent(view, &event);
        };

        const double width = chart->xAxis->range().size();
        QWheelEvent wheel(QPointF(centre), QPointF(view->mapToGlobal(centre)), QPoint(), QPoint(0, 120), Qt::NoButton, Qt::NoModifier,
                          Qt::NoScrollPhase, false);
        QApplication::sendEvent(view, &wheel);
        QVERIFY(chart->xAxis->range().size() < width); // zoomed in

        find<QCheckBox>("checkBoxAutoTrack")->setChecked(true);
        const double lower = chart->xAxis->range().lower;
        mouse(QEvent::MouseButtonPress, centre, Qt::LeftButton, Qt::LeftButton);
        mouse(QEvent::MouseMove, centre + QPoint(250, 0), Qt::NoButton, Qt::LeftButton);
        mouse(QEvent::MouseButtonRelease, centre + QPoint(250, 0), Qt::LeftButton, Qt::NoButton);
        QVERIFY(chart->xAxis->range().lower < lower);                 // dragged to the right = earlier data
        QVERIFY(!find<QCheckBox>("checkBoxAutoTrack")->isChecked()); // long drag stops auto-scroll

        mouse(QEvent::MouseButtonPress, centre - QPoint(40, 30), Qt::MiddleButton, Qt::MiddleButton);
        mouse(QEvent::MouseMove, centre + QPoint(40, 30), Qt::NoButton, Qt::MiddleButton);
        const double beforeRect = chart->xAxis->range().size();
        mouse(QEvent::MouseButtonRelease, centre + QPoint(40, 30), Qt::MiddleButton, Qt::NoButton);
        QVERIFY(chart->xAxis->range().size() < beforeRect); // rectangle zoom

        // Fit all data: the X range then covers exactly the recorded data.
        QTest::mouseDClick(view, Qt::LeftButton, {}, centre);
        bool found = false;
        const QCPRange data = chart->graph(0)->getKeyRange(found);
        QVERIFY(found);
        QVERIFY(qAbs(chart->xAxis->range().lower - data.lower) < 1e-6);
        QVERIFY(qAbs(chart->xAxis->range().upper - data.upper) < 1e-6);

        window->applyPreferences(AppPreferences()); // back to QCustomPlot
        QVERIFY(!view->isVisible());
        QVERIFY(find<QCustomPlot>("widgetChart")->isVisible());
        find<QPushButton>("pushButtonSerialConnect")->setChecked(false);
#else
        QSKIP("Built without Qt Graphs");
#endif
    }

    void qtGraphsFollowsManualChanges() // clearing the chart used to leave the old lines on screen
    {
#ifdef QTSM_HAS_QT_GRAPHS
        AppPreferences graphs;
        graphs.chartEngine = AppPreferences::ChartEngine::QtGraphs;
        window->applyPreferences(graphs);
        auto *view = window->findChild<GraphsChartView *>("graphsChartView");
        if (!view)
            QSKIP("Qt Graphs could not be started");

        send("x 1 y 2");
        QTRY_COMPARE(view->seriesCount(), 2);
        find<QPushButton>("pushButtonPauseChart")->setChecked(true); // no periodic redraws while paused
        find<QPushButton>("pushButtonClearAll")->click();
        QCOMPARE(view->seriesCount(), 0);

        find<QPushButton>("pushButtonPauseChart")->setChecked(false);

        // The Samples marker is mirrored.
        send("x 5");
        QTRY_VERIFY(find<QCustomPlot>("widgetChart")->graphCount() > 0); // Clear All removed the graphs
        showSamples();
        QTRY_VERIFY(logTable()->rowCount() > 0);
        find<QTableView>("tableWidgetLogTable")->setCurrentIndex(logTable()->index(0, 0));
        QVERIFY(!qIsNaN(view->rootObject()->property("markerX").toDouble()));
        find<QTableView>("tableWidgetLogTable")->setCurrentIndex(QModelIndex());
        QVERIFY(qIsNaN(view->rootObject()->property("markerX").toDouble()));

        // Chart shortcuts work on the Qt Graphs chart too.
        view->setFocus();
        QVERIFY(view->hasFocus());
        auto *autoTrack = find<QCheckBox>("checkBoxAutoTrack");
        const bool tracking = autoTrack->isChecked();
        QTest::keyClick(view, Qt::Key_X);
        QCOMPARE(autoTrack->isChecked(), !tracking);
        QTest::keyClick(view, Qt::Key_Space);
        QVERIFY(find<QPushButton>("pushButtonPauseChart")->isChecked());
        QTest::keyClick(view, Qt::Key_Space);
        autoTrack->setChecked(tracking);

        // Export uses the size of the chart on screen; the hidden QCustomPlot is not resized with the window.
        window->resize(window->size() + QSize(200, 150));
        QTRY_VERIFY(view->width() > 0);
        QTemporaryDir dir;
        QVERIFY(window->saveChartImage(dir.filePath("chart.png")));
        QCOMPARE(QImage(dir.filePath("chart.png")).size(), view->size());

        window->applyPreferences(AppPreferences());
#else
        QSKIP("Built without Qt Graphs");
#endif
    }

    void autoLogWritesToLogFolder()
    {
        QTemporaryDir logs;
        find<QPushButton>("pushButtonUDPConnect")->setChecked(false);
        find<QLineEdit>("lineEditSaveLogPath")->setText(logs.filePath("my logs"));
        find<QCheckBox>("checkBoxAutoLogging")->setChecked(true);

        window->startDemo(100);
        QTest::qWait(300);
        find<QPushButton>("pushButtonSerialConnect")->setChecked(false); // closes the log file
        find<QCheckBox>("checkBoxAutoLogging")->setChecked(false);

        const QStringList files = QDir(logs.filePath("my logs")).entryList(QDir::Files);
        QCOMPARE(files.count(), 1);
        QVERIFY2(files[0].contains("_Log."), qPrintable(files[0])); // date-stamped name
        QVERIFY(QFileInfo(logs.filePath("my logs/" + files[0])).size() > 100);
    }

    void logFileNameIsKept() // auto-logging used to wipe the file name; the format switch replaced "txt" anywhere in it
    {
        auto *name = find<QLineEdit>("lineEditSaveFileName");
        auto *format = find<QComboBox>("comboBoxLogFormat");
        format->setCurrentIndex(format->findText(".txt"));
        name->setText("my_txt_data.txt");

        format->setCurrentIndex(format->findText(".csv"));
        QCOMPARE(name->text(), QString("my_txt_data.csv"));

        find<QCheckBox>("checkBoxAutoLogging")->setChecked(true);
        QVERIFY(name->text().startsWith("%DateTime%"));
        find<QCheckBox>("checkBoxAutoLogging")->setChecked(false);
        QCOMPARE(name->text(), QString("my_txt_data.csv"));

        name->setText("other.csv");
        QVERIFY(QMetaObject::invokeMethod(window, "on_lineEditSaveFileName_editingFinished"));
        QCOMPARE(name->text(), QString("other.csv")); // not other.csv.csv
        format->setCurrentIndex(format->findText(".txt"));
        QCOMPARE(name->text(), QString("other.txt"));
    }

    void pointMarkersCheckbox() // issue #11
    {
        send("m 1 n 2");
        QTRY_COMPARE(graphNames().count(), 2);
        QCPGraph *graph = find<QCustomPlot>("widgetChart")->graph(0);
        auto *markers = find<QCheckBox>("checkBoxShowPointMarkers");

        QVERIFY(!markers->isChecked()); // plain lines by default
        QCOMPARE(graph->scatterStyle().shape(), QCPScatterStyle::ssNone);
        markers->setChecked(true);
        QCOMPARE(graph->scatterStyle().shape(), QCPScatterStyle::ssDisc);
        QCOMPARE(find<QCustomPlot>("widgetChart")->graph(1)->scatterStyle().shape(), QCPScatterStyle::ssDisc); // all graphs, same marker
        markers->setChecked(false);
        QCOMPARE(graph->scatterStyle().shape(), QCPScatterStyle::ssNone);
    }

    void plotOnlyLabelsWithSpaces() // issue #10
    {
        auto *rules = find<QLineEdit>("lineEditCustomParsingRules");
        find<QComboBox>("comboBoxGraphDisplayMode")->setCurrentIndex(1); // Include
        rules->setText("Graph 0, Graph 10");
        QVERIFY(QMetaObject::invokeMethod(window, "on_lineEditCustomParsingRules_editingFinished"));
        QCOMPARE(rules->text(), QString("Graph 0 || Graph 10")); // not "Graph || 0 || ..."

        send("0 1 2 3 4 5 6 7 8 9 10 11"); // Graph 0 ... Graph 11
        QTRY_VERIFY(!graphNames().isEmpty());
        QTest::qWait(100);
        QCOMPARE(graphNames(), (QStringList{"Graph 0", "Graph 10"})); // exact matches only, no "Graph 1"

        find<QComboBox>("comboBoxGraphDisplayMode")->setCurrentIndex(0);
        rules->clear();
    }

    void highRatePointsDoNotPileUp() // issue #8
    {
        find<QPushButton>("pushButtonUDPConnect")->setChecked(false);
        find<QSpinBox>("spinBoxProcessingDelay")->setValue(50); // batches of ~25 lines at 500 Hz
        window->startDemo(500);
        QCPGraph *graph = nullptr;
        QTRY_VERIFY((graph = find<QCustomPlot>("widgetChart")->graph(0)) && graph->dataCount() > 300);
        find<QPushButton>("pushButtonSerialConnect")->setChecked(false);
        find<QSpinBox>("spinBoxProcessingDelay")->setValue(1);

        int sameTime = 0;
        for (int i = 1; i < graph->dataCount(); ++i)
            if (graph->dataMainKey(i) == graph->dataMainKey(i - 1))
                ++sameTime;
        QVERIFY2(sameTime < graph->dataCount() / 10, qPrintable(QString("%1 of %2 points share a time stamp").arg(sameTime).arg(graph->dataCount())));
    }

    void customBaudRateIsKept() // issue #5
    {
        auto *baud = find<QComboBox>("comboBoxBaudRates");
        QVERIFY(baud->isEditable());

        int position = 0;
        QString text = "12x34";
        QCOMPARE(baud->validator()->validate(text, position), QValidator::Invalid); // numbers only

        baud->setCurrentText("2000000"); // not in the standard list
        QVERIFY(QMetaObject::invokeMethod(window, "on_aboutToQuitSlot"));

        MainWindow restarted;
        QCOMPARE(restarted.findChild<QComboBox *>("comboBoxBaudRates")->currentText(), QString("2000000"));
    }

    void whatsNewShowsChangelog()
    {
        QAction *whatsNew = window->findChild<QAction *>("actionWhat_s_new");
        QVERIFY(whatsNew && whatsNew->isEnabled());
        whatsNew->trigger();

        QTextBrowser *changelog = nullptr; // the info window is a separate top-level window
        for (QWidget *topLevel : QApplication::topLevelWidgets())
            if (!changelog)
                changelog = topLevel->findChild<QTextBrowser *>("textBrowserChangelog");
        QVERIFY(changelog);
        QVERIFY(changelog->isVisible());
        const QString text = changelog->toPlainText();
        QVERIFY(text.contains("2.0.0"));
        QVERIFY(text.contains("1.6.0"));
        QVERIFY(!text.contains("##")); // rendered, not raw Markdown
        changelog->window()->close();
    }

    void importFormatFollowsExtension() // the logging format used to decide how a file was read
    {
        QTemporaryDir dir;
        QFile csv(dir.filePath("data.CSV"));
        QVERIFY(csv.open(QIODevice::WriteOnly));
        csv.write("\"time\",\"a\"\n2024-01-01T10:00:00.000Z,1\n2024-01-01T10:00:01.000Z,2\n");
        csv.close();

        find<QComboBox>("comboBoxLogFormat")->setCurrentIndex(1); // TXT logging
        QVERIFY(window->importFile(csv.fileName()));
        QTRY_COMPARE(graphNames(), QStringList{"a"});
        QCOMPARE(find<QCustomPlot>("widgetChart")->graph(0)->dataCount(), 2);
        find<QComboBox>("comboBoxLogFormat")->setCurrentIndex(0);
    }

    void highlightIsReplaced() // old highlights used to stay when searching for something else
    {
        auto *text = find<QPlainTextEdit>("textBrowserLogs");
        text->setPlainText("alpha beta alpha");
        auto formatAt = [text](int position)
        {
            QTextCursor cursor(text->document());
            cursor.setPosition(position + 1); // format of the character before the cursor
            return cursor.charFormat();
        };

        find<QLineEdit>("lineEditHighlight")->setText("alpha");
        QVERIFY(QMetaObject::invokeMethod(window, "on_lineEditHighlight_returnPressed"));
        QCOMPARE(formatAt(0).fontWeight(), int(QFont::Bold));
        QCOMPARE(formatAt(11).fontWeight(), int(QFont::Bold));
        QVERIFY(formatAt(7).fontWeight() != int(QFont::Bold));

        find<QLineEdit>("lineEditHighlight")->setText("beta");
        QVERIFY(QMetaObject::invokeMethod(window, "on_lineEditHighlight_returnPressed"));
        QVERIFY(formatAt(0).fontWeight() != int(QFont::Bold));
        QVERIFY(!formatAt(0).hasProperty(QTextFormat::BackgroundBrush));
        QCOMPARE(formatAt(7).fontWeight(), int(QFont::Bold));
        find<QLineEdit>("lineEditHighlight")->clear();
    }

    void shortDragKeepsAutoScroll() // a drag started far from where the previous one ended used to count the gap
    {
        auto *chart = find<QCustomPlot>("widgetChart");
        auto *autoTrack = find<QCheckBox>("checkBoxAutoTrack");
        autoTrack->setChecked(true);
        auto drag = [chart](int fromX, int toX)
        {
            const int y = chart->height() / 2;
            QTest::mousePress(chart, Qt::LeftButton, {}, QPoint(fromX, y));
            QMouseEvent move(QEvent::MouseMove, QPointF(toX, y), QPointF(chart->mapToGlobal(QPoint(toX, y))), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(chart, &move);
            QTest::mouseRelease(chart, Qt::LeftButton, {}, QPoint(toX, y));
        };

        drag(chart->width() - 40, chart->width() - 30); // short drags, far apart
        drag(30, 40);
        QVERIFY(autoTrack->isChecked());

        drag(30, 30 + 250); // a long one
        QVERIFY(!autoTrack->isChecked());
        autoTrack->setChecked(true);
    }

    void resetStatisticsClearsTable() // the old numbers used to stay until new data arrived
    {
        auto *table = find<QTableWidget>("tableWidgetParsedData");
        send("v 1");
        QTest::qWait(30);
        send("v 3");
        QTRY_VERIFY(table->rowCount() == 1 && table->item(0, 5) && table->item(0, 5)->text() == "2"); // Count
        QCOMPARE(table->item(0, 3)->text(), QString("3"));                                           // Max

        QVERIFY(QMetaObject::invokeMethod(window, "resetChannelStatistics"));
        QCoreApplication::processEvents();
        QCOMPARE(table->item(0, 5)->text(), QString());
        QCOMPARE(table->item(0, 3)->text(), QString());
        QCOMPARE(table->item(0, 1)->text(), QString("3")); // last value stays

        send("v 2");
        QTRY_COMPARE(table->item(0, 5)->text(), QString("1"));
        QCOMPARE(table->item(0, 3)->text(), QString("2"));
    }

    void importFromFileMenu() // replaces the Load file tab
    {
        auto *import = window->findChild<QAction *>("actionImport");
        QVERIFY(import && import->isEnabled());
        QCOMPARE(import->shortcut(), QKeySequence(QKeySequence::Open));
        QVERIFY(!window->findChild<QWidget *>("tabLoadFile"));

        QTemporaryDir dir;
        QFile text(dir.filePath("log.txt"));
        QVERIFY(text.open(QIODevice::WriteOnly));
        for (int i = 0; i < 5; ++i)
            text.write(QString("10:00:0%1.000 u %2 v %3\n").arg(i).arg(i).arg(-i).toUtf8());
        text.close();

        showSamples();
        QVERIFY(window->importFile(text.fileName()));
        QTRY_VERIFY(logText().contains("Imported 10 values from log.txt"));
        QCOMPARE(graphNames(), (QStringList{"u", "v"}));
        QTRY_COMPARE(logTable()->rowCount(), 5); // the Samples table shows the imported data (next chart tick)

        QVERIFY(!window->importFile(dir.filePath("missing.csv")));
        QVERIFY(logText().contains("unable to import"));

        // Big import: Cancel in the progress dialog stops it.
        QFile big(dir.filePath("big.txt"));
        QVERIFY(big.open(QIODevice::WriteOnly));
        QByteArray chunk;
        for (int i = 0; i < 1000; ++i)
            chunk += "a 1 b 2 c 3 d 4\n";
        for (int i = 0; i < 1000; ++i)
            big.write(chunk);
        big.close();
        QVERIFY(window->importFile(big.fileName()));
        auto *progress = window->findChild<QProgressDialog *>("importProgress");
        QVERIFY(progress);
        QPushButton *cancel = progress->findChild<QPushButton *>();
        QVERIFY(cancel);
        cancel->click();
        QTRY_VERIFY(logText().contains("Import cancelled"));
        QTRY_VERIFY(!window->findChild<QProgressDialog *>("importProgress"));
    }

    void tracerToggleAndClearAll() // used to leave dangling pointers inside QCustomPlot
    {
        send("x 1 y 2");
        QTRY_COMPARE(graphNames().count(), 2);

        auto *tracer = find<QCheckBox>("checkBoxEnableTracer");
        tracer->setChecked(true);
        tracer->setChecked(false);
        tracer->setChecked(true);
        find<QPushButton>("pushButtonClearAll")->click();
        find<QCustomPlot>("widgetChart")->replot();

        send("x 3");
        QTRY_COMPARE(graphNames(), QStringList{"x"});
    }
};

QTEST_MAIN(TestMainWindow)
#include "tst_mainwindow.moc"
