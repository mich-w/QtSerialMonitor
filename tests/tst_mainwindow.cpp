// Integration test: drives the real MainWindow headlessly and feeds it data over UDP.
#include "mainwindow.h"
#include "theme.h"
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSpinBox>
#include <QToolButton>
#include <QTableWidget>

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
        find<QPushButton>("pushButtonEnableTableLog")->setChecked(false);  // checked == disabled
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
        QCOMPARE(table->item(0, 1)->text(), QString("9"));
        QCOMPARE(table->item(1, 1)->text(), QString("100"));

        // Statistics columns for "b" (values 2, 5, 6, 7, 8, 9)
        QCOMPARE(table->horizontalHeaderItem(2)->text(), QString("Min"));
        QCOMPARE(table->item(0, 2)->text(), QString("2"));
        QCOMPARE(table->item(0, 3)->text(), QString("9"));
        QCOMPARE(table->item(0, 4)->text(), QString::number(37.0 / 6.0));
        QCOMPARE(table->item(0, 5)->text(), QString("6"));

        auto *logTable = find<QTableWidget>("tableWidgetLogTable");
        QCOMPARE(logTable->rowCount(), 6);
        QCOMPARE(logTable->horizontalHeaderItem(1)->text(), QString("a"));
        QCOMPARE(logTable->item(0, 1)->text(), QString("1")); // first value of a new label is kept
    }

    void logTableKeepsLinesFromOneBatch()
    {
        find<QSpinBox>("spinBoxProcessingDelay")->setValue(200); // force both lines into one batch
        send("x 1 y 2");
        send("x 3 y 4");

        auto *logTable = find<QTableWidget>("tableWidgetLogTable");
        QTRY_COMPARE(logTable->rowCount(), 2);
        QCOMPARE(logTable->item(0, 1)->text(), QString("1"));
        QCOMPARE(logTable->item(1, 1)->text(), QString("3"));
        QCOMPARE(logTable->item(1, 2)->text(), QString("4"));
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
