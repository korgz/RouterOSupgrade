#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "upgradeworker.h"

#include <QThread>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    connect(ui->startButton, &QPushButton::clicked, this, &MainWindow::onStartClicked);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::onStartClicked() {
    QString version = ui->versionEdit->text();
    QString ipPrefix = ui->ipPrefixEdit->text();
    QString range = ui->rangeEdit->text();
    bool debug = ui->debugCheck->isChecked();
    bool downgrade = ui->downgradeCheck->isChecked();

    if (version.isEmpty() || ipPrefix.isEmpty() || range.isEmpty()) {
        QMessageBox::warning(this, "Missing Input", "Please fill all fields.");
        return;
    }

    auto *worker = new UpgradeWorker(version, ipPrefix, range, debug, downgrade);
    auto *thread = new QThread;

    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, &UpgradeWorker::run);
    connect(worker, &UpgradeWorker::finished, thread, &QThread::quit);
    connect(worker, &UpgradeWorker::finished, worker, &UpgradeWorker::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    connect(worker, &UpgradeWorker::logOutput, this, [=](const QString &text) {
        ui->outputTextEdit->append(text);  // or .insertPlainText(text);
    });

    thread->start();
}
