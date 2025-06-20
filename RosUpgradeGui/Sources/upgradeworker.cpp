#include "upgradeworker.h"
#include <QProcess>
#include <QDebug>
#include <QCoreApplication>

UpgradeWorker::UpgradeWorker(const QString &version, const QString &prefix,
                             const QString &range, bool debug, bool downgrade)
    : rosVersion(version), ipPrefix(prefix), ipRange(range),
      debugMode(debug), downgradeMode(downgrade) {}

void UpgradeWorker::run() {
    QStringList args;
    args << rosVersion << ipPrefix << ipRange;

    if (debugMode) args << "--debug";
    if (downgradeMode) args << "-d";

    QProcess process;
    QString backendPath = QCoreApplication::applicationDirPath() + "/upgrade";
    process.setProgram(backendPath);
    process.setArguments(args);
    process.setProcessChannelMode(QProcess::MergedChannels);

    connect(&process, &QProcess::readyReadStandardOutput, [&]() {
        QByteArray data = process.readAllStandardOutput();
        emit logOutput(QString::fromUtf8(data));
    });

    process.start();
    process.waitForFinished(-1);

    emit finished();
}


