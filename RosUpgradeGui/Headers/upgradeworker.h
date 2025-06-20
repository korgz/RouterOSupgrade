#ifndef UPGRADEWORKER_H
#define UPGRADEWORKER_H

#include <QObject>
#include <QString>

class UpgradeWorker : public QObject {
    Q_OBJECT

public:
    UpgradeWorker(const QString &version, const QString &prefix,
                  const QString &range, bool debug, bool downgrade);

public slots:
    void run();

signals:
    void finished();
    void logOutput(const QString &text);

private:
    QString rosVersion, ipPrefix, ipRange;
    bool debugMode, downgradeMode;
};

#endif // UPGRADEWORKER_H
