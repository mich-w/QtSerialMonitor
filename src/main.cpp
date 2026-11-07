#include "config.h"
#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>

#include <cstdio>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("QtSerialMonitor");
    QApplication::setApplicationVersion(VERSION);
    app.setStyle("fusion");

    QCommandLineParser parser;
    parser.setApplicationDescription("Serial monitor with data plotting.");
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption demoOption("demo", "Start with the simulated device streaming sine waves.");
    const QCommandLineOption demoRateOption("demo-rate", "Samples per second sent by the demo device (1-1000, default 50).", "hz",
                                            QString::number(DemoTransport::DefaultRateHz));
    parser.addOption(demoOption);
    parser.addOption(demoRateOption);
    parser.process(app);

    const bool demo = parser.isSet(demoOption) || parser.isSet(demoRateOption);
    bool rateOk = false;
    const int demoRate = parser.value(demoRateOption).toInt(&rateOk);
    if (demo && (!rateOk || demoRate < 1 || demoRate > 1000))
    {
        std::fputs("--demo-rate must be a number between 1 and 1000.\n", stderr);
        return 1;
    }

    MainWindow window;
    window.show();

    if (demo)
        window.startDemo(demoRate);

    return app.exec();
}
