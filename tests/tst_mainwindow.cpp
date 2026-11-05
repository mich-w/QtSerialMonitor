// Integration test: drives the real MainWindow headlessly and feeds it data over UDP.
#include "mainwindow.h"
#include <QCheckBox>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
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

        auto *logTable = find<QTableWidget>("tableWidgetLogTable");
        QCOMPARE(logTable->rowCount(), 6);
        QCOMPARE(logTable->horizontalHeaderItem(1)->text(), QString("a"));
        QCOMPARE(logTable->item(0, 1)->text(), QString("1")); // first value of a new label is kept
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
