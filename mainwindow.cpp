#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "ProcessingWorker.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QThread>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    processingTimer = new QTimer(this);
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);
    ui->progressBar->setTextVisible(true);
    ui->progressBar->setFormat("%p%");

    connect(
        processingTimer,
        &QTimer::timeout,
        this,
        &MainWindow::scanAndStartProcessing
        );

    connect(
        ui->startButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            if (!validateInput()) {
                return;
            }

            if (processingThread != nullptr) {
                return;
            }

            const bool timerMode =
                ui->timerRadioButton->isChecked();

            if (timerMode) {
                const int interval =
                    ui->timerIntervalSpinBox->value();

                processingTimer->start(interval);

                ui->startButton->setEnabled(false);
                ui->stopButton->setEnabled(true);
            }

            scanAndStartProcessing();
        }
        );

    connect(
        ui->pauseButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            if (processingWorker == nullptr) {
                return;
            }

            if (ui->pauseButton->text() == "Pause") {
                ui->pauseButton->setEnabled(false);

                ui->statusLabel->setText(
                    "Status: Pausing..."
                    );

                processingWorker->requestPause();
            } else {
                ui->pauseButton->setEnabled(false);

                ui->statusLabel->setText(
                    "Status: Resuming..."
                    );

                processingWorker->requestResume();
            }
        }
        );

    connect(
        ui->stopButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            if (processingTimer->isActive()) {
                processingTimer->stop();
            }

            if (processingWorker != nullptr) {
                processingWorker->requestStop();

                ui->statusLabel->setText(
                    "Status: Stopping..."
                    );

                ui->stopButton->setEnabled(false);
                ui->pauseButton->setEnabled(false);
            } else {
                ui->statusLabel->setText(
                    "Status: Timer stopped"
                    );

                ui->startButton->setEnabled(true);
                ui->stopButton->setEnabled(false);
                ui->pauseButton->setEnabled(false);
                ui->pauseButton->setText("Pause");
            }
        }
        );

    ui->fileMaskEdit->setMaxLength(255);

    QRegularExpression hexRegex(
        "^[0-9A-Fa-f]{0,16}$"
        );

    ui->xorValueEdit->setValidator(
        new QRegularExpressionValidator(
            hexRegex,
            ui->xorValueEdit
            )
        );

    connect(
        ui->browseInputButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            const QString path =
                QFileDialog::getExistingDirectory(
                    this,
                    "Select input directory"
                    );

            if (!path.isEmpty()) {
                ui->inputPathEdit->setText(path);
            }
        }
        );

    connect(
        ui->browseOutputButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            const QString path =
                QFileDialog::getExistingDirectory(
                    this,
                    "Select output directory"
                    );

            if (!path.isEmpty()) {
                ui->outputPathEdit->setText(path);
            }
        }
        );

    auto updateTimerControls = [this]()
    {
        const bool timerMode =
            ui->timerRadioButton->isChecked();

        ui->timerIntervalLabel->setEnabled(timerMode);
        ui->timerIntervalSpinBox->setEnabled(timerMode);
    };

    connect(
        ui->singleRunRadioButton,
        &QRadioButton::toggled,
        this,
        [updateTimerControls](bool)
        {
            updateTimerControls();
        }
        );

    connect(
        ui->timerRadioButton,
        &QRadioButton::toggled,
        this,
        [updateTimerControls](bool)
        {
            updateTimerControls();
        }
        );

    updateTimerControls();
}

void MainWindow::scanAndStartProcessing()
{
    if (processingThread != nullptr) {
        return;
    }

    const bool timerMode =
        ui->timerRadioButton->isChecked();

    currentRunTimerMode = timerMode;

    if (!validateInput()) {
        return;
    }

    currentProcessingFileStates.clear();

    const QString inputPath =
        ui->inputPathEdit->text().trimmed();

    const QString outputPath =
        ui->outputPathEdit->text().trimmed();

    bool ok = false;

    const quint64 xorValue =
        ui->xorValueEdit->text().toULongLong(
            &ok,
            16
            );

    if (!ok) {
        ui->statusLabel->setText(
            "Status: Invalid XOR value"
            );

        return;
    }

    const QString maskText =
        ui->fileMaskEdit->text().trimmed();

    QStringList masks;

    for (const QString& mask :
         maskText.split(';', Qt::SkipEmptyParts)) {

        const QString trimmedMask =
            mask.trimmed();

        if (!trimmedMask.isEmpty()) {
            masks.append(trimmedMask);
        }
    }

    QDir inputDirectory(inputPath);

    const QFileInfoList files =
        inputDirectory.entryInfoList(
            masks,
            QDir::Files | QDir::Readable,
            QDir::Name
            );

    if (files.isEmpty()) {
        ui->statusLabel->setText(
            "Status: No matching files found"
            );

        return;
    }

    QStringList inputPaths;

    for (const QFileInfo& file : files) {
        const QString filePath =
            file.absoluteFilePath();

        const FileState currentState{
            file.size(),
            file.lastModified()
        };

        if (currentRunTimerMode) {
            const auto processedIt =
                processedInputFiles.constFind(filePath);

            if (processedIt != processedInputFiles.constEnd()) {
                const FileState& processedState =
                    processedIt.value();

                if (processedState.size == currentState.size
                    && processedState.lastModified == currentState.lastModified) {
                    continue;
                }
            }
        }

        inputPaths.append(filePath);

        currentProcessingFileStates.insert(
            filePath,
            currentState
            );
    }

    if (inputPaths.isEmpty()) {
        ui->statusLabel->setText(
            "Status: No new matching files found"
            );

        return;
    }

    ui->statusLabel->setText(
        QString("Status: Found %1 file(s)")
            .arg(inputPaths.size())
        );

    const bool overwrite =
        ui->overwriteRadioButton->isChecked();

    const bool deleteInputFiles =
        ui->deleteInputCheckBox->isChecked();

    ui->progressBar->setValue(0);

    ui->startButton->setEnabled(false);

    ui->pauseButton->setText("Pause");
    ui->pauseButton->setEnabled(true);

    ui->stopButton->setEnabled(true);

    processingThread = new QThread(this);

    processingWorker = new ProcessingWorker();

    processingWorker->moveToThread(
        processingThread
        );

    connect(
        this,
        &MainWindow::startProcessing,
        processingWorker,
        &ProcessingWorker::processFiles,
        Qt::QueuedConnection
        );

    connect(
        processingWorker,
        &ProcessingWorker::progressChanged,
        this,
        [this](int percent)
        {
            ui->progressBar->setValue(percent);
            ui->progressBar->repaint();
        },
        Qt::QueuedConnection
        );

    connect(
        processingWorker,
        &ProcessingWorker::paused,
        this,
        [this]()
        {
            ui->statusLabel->setText(
                "Status: Paused"
                );

            ui->pauseButton->setText("Resume");
            ui->pauseButton->setEnabled(true);
        }
        );

    connect(
        processingWorker,
        &ProcessingWorker::resumed,
        this,
        [this]()
        {
            ui->statusLabel->setText(
                "Status: Processing..."
                );

            ui->pauseButton->setText("Pause");
            ui->pauseButton->setEnabled(true);
        }
        );

    connect(
        processingWorker,
        &ProcessingWorker::finished,
        this,
        [this](bool success)
        {
            if (success) {
                if (currentRunTimerMode) {
                    for (auto it = currentProcessingFileStates.cbegin();
                         it != currentProcessingFileStates.cend();
                         ++it) {

                        processedInputFiles.insert(
                            it.key(),
                            it.value()
                            );
                    }
                }

                ui->statusLabel->setText(
                    "Status: Processing completed"
                    );

                ui->progressBar->setValue(100);
            } else {
                ui->statusLabel->setText(
                    "Status: Processing failed"
                    );
            }

            currentProcessingFileStates.clear();

            ui->pauseButton->setEnabled(false);
            ui->pauseButton->setText("Pause");

            if (!processingTimer->isActive()) {
                ui->startButton->setEnabled(true);
                ui->stopButton->setEnabled(false);
            }

            processingThread->quit();
        }
        );

    connect(
        processingWorker,
        &ProcessingWorker::stopped,
        this,
        [this]()
        {
            currentProcessingFileStates.clear();

            ui->statusLabel->setText(
                "Status: Processing stopped"
                );

            ui->startButton->setEnabled(true);
            ui->stopButton->setEnabled(false);

            ui->pauseButton->setEnabled(false);
            ui->pauseButton->setText("Pause");

            processingTimer->stop();

            processingThread->quit();
        }
        );

    connect(
        processingWorker,
        &ProcessingWorker::finished,
        processingWorker,
        &QObject::deleteLater
        );

    connect(
        processingWorker,
        &ProcessingWorker::stopped,
        processingWorker,
        &QObject::deleteLater
        );

    connect(
        processingThread,
        &QThread::finished,
        processingThread,
        &QObject::deleteLater
        );

    connect(
        processingThread,
        &QThread::finished,
        this,
        [this]()
        {
            processingThread = nullptr;
            processingWorker = nullptr;
        }
        );

    processingThread->start();

    emit startProcessing(
        inputPaths,
        outputPath,
        xorValue,
        overwrite,
        deleteInputFiles
        );
}

bool MainWindow::validateInput()
{
    const QString inputPath =
        ui->inputPathEdit->text().trimmed();

    if (inputPath.isEmpty()) {
        ui->statusLabel->setText(
            "Status: Input directory is not selected"
            );

        return false;
    }

    if (!QDir(inputPath).exists()) {
        ui->statusLabel->setText(
            "Status: Input directory does not exist"
            );

        return false;
    }

    const QString outputPath =
        ui->outputPathEdit->text().trimmed();

    if (outputPath.isEmpty()) {
        ui->statusLabel->setText(
            "Status: Output directory is not selected"
            );

        return false;
    }

    if (!QDir(outputPath).exists()) {
        ui->statusLabel->setText(
            "Status: Output directory does not exist"
            );

        return false;
    }

    const QString canonicalInputPath =
        QDir(inputPath).canonicalPath();

    const QString canonicalOutputPath =
        QDir(outputPath).canonicalPath();

    if (canonicalInputPath == canonicalOutputPath) {
        ui->statusLabel->setText(
            "Status: Input and output directories must be different"
            );

        return false;
    }

    const QString maskText =
        ui->fileMaskEdit->text().trimmed();

    if (maskText.isEmpty()) {
        ui->statusLabel->setText(
            "Status: File mask is empty"
            );

        return false;
    }

    QStringList masks;

    for (const QString& mask :
         maskText.split(';', Qt::SkipEmptyParts)) {

        QString trimmedMask = mask.trimmed();

        if (trimmedMask.isEmpty()) {
            continue;
        }

        if (trimmedMask.startsWith('.')
            && !trimmedMask.contains('*')
            && !trimmedMask.contains('?')) {

            trimmedMask = "*" + trimmedMask;
        }

        masks.append(trimmedMask);
    }

    if (masks.isEmpty()) {
        ui->statusLabel->setText(
            "Status: No valid file masks specified"
            );

        return false;
    }

    const QString xorValue =
        ui->xorValueEdit->text();

    if (xorValue.length() != 16) {
        ui->statusLabel->setText(
            "Status: XOR value must contain exactly 16 hex characters"
            );

        return false;
    }

    ui->statusLabel->setText(
        "Status: Configuration is valid"
        );

    return true;
}

MainWindow::~MainWindow()
{
    if (processingTimer != nullptr) {
        processingTimer->stop();
    }

    if (processingWorker != nullptr) {
        processingWorker->requestStop();
    }

    if (processingThread != nullptr) {
        processingThread->quit();
        processingThread->wait();
    }

    delete ui;
}