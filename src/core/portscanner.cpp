#include "portscanner.h"

#include <memory>

PortScanner::PortScanner(QObject *parent) : QObject(parent)
{
}

PortScanner::~PortScanner()
{
    if (worker)
    {
        worker->wait();
        delete worker;
    }
}

bool PortScanner::isScanning() const
{
    return !worker.isNull();
}

void PortScanner::scan()
{
    if (isScanning())
        return;

    auto result = std::make_shared<QList<PortIdentity>>();
    worker = QThread::create([result]()
                             { *result = PortIdentity::available(); });

    connect(worker, &QThread::finished, this, [this, result]()
            {
                worker->deleteLater();
                worker = nullptr;
                emit scanned(*result);
            });
    worker->start(QThread::LowPriority);
}
