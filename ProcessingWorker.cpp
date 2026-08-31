#include "ProcessingWorker.h"
#include "FileProcessor.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>

ProcessingWorker::ProcessingWorker(QObject *parent)
    : QObject(parent)
{
}

void ProcessingWorker::requestStop()
{
    stopRequested.store(true);
    pauseRequested.store(false);

    pauseCondition.notify_all();
}

void ProcessingWorker::requestPause()
{
    pauseRequested.store(true);
}

void ProcessingWorker::requestResume()
{
    pauseRequested.store(false);

    pauseCondition.notify_all();
}

bool ProcessingWorker::waitIfPaused()
{
    std::unique_lock<std::mutex> lock(pauseMutex);

    if (!pauseRequested.load()) {
        return !stopRequested.load();
    }

    emit paused();

    pauseCondition.wait(
        lock,
        [this]()
        {
            return !pauseRequested.load()
            || stopRequested.load();
        }
        );

    if (stopRequested.load()) {
        return false;
    }

    emit resumed();

    return true;
}

void ProcessingWorker::processFiles(
    const QStringList& inputPaths,
    const QString& outputDirectory,
    quint64 xorValue,
    bool overwrite,
    bool deleteInputFiles)
{
    stopRequested.store(false);
    pauseRequested.store(false);

    if (inputPaths.isEmpty()) {
        emit finished(false);
        return;
    }

    const int totalFiles = inputPaths.size();
    int processedFiles = 0;

    for (const QString& inputPath : inputPaths) {
        if (stopRequested.load()) {
            emit stopped();
            return;
        }

        const QFileInfo inputFile(inputPath);

        QString outputFilePath =
            QDir(outputDirectory).filePath(
                inputFile.fileName()
                );

        if (!overwrite && QFile::exists(outputFilePath)) {
            const QString baseName =
                inputFile.completeBaseName();

            const QString suffix =
                inputFile.completeSuffix();

            int counter = 1;

            do {
                QString fileName;

                if (suffix.isEmpty()) {
                    fileName =
                        QString("%1_%2")
                            .arg(baseName)
                            .arg(counter);
                } else {
                    fileName =
                        QString("%1_%2.%3")
                            .arg(baseName)
                            .arg(counter)
                            .arg(suffix);
                }

                outputFilePath =
                    QDir(outputDirectory).filePath(fileName);

                ++counter;

            } while (QFile::exists(outputFilePath));
        }

        const QString absoluteInputPath =
            QFileInfo(inputPath).absoluteFilePath();

        const QString absoluteOutputPath =
            QFileInfo(outputFilePath).absoluteFilePath();

        if (absoluteInputPath == absoluteOutputPath) {
            emit finished(false);
            return;
        }

        QTemporaryFile temporaryFile(
            QDir(outputDirectory).filePath(
                "." + inputFile.fileName() + ".XXXXXX.part"
                )
            );

        temporaryFile.setAutoRemove(false);

        if (!temporaryFile.open()) {
            emit finished(false);
            return;
        }

        const QString temporaryFilePath =
            temporaryFile.fileName();

        temporaryFile.close();

        FileProcessor processor;

        const bool success = processor.processFile(
            inputPath,
            temporaryFilePath,
            xorValue,
            [this, processedFiles, totalFiles]
            (qint64 processedBytes, qint64 totalBytes)
            {
                if (totalBytes <= 0) {
                    return;
                }

                const double fileProgress =
                    static_cast<double>(processedBytes)
                    / static_cast<double>(totalBytes);

                const double overallProgress =
                    (
                        static_cast<double>(processedFiles)
                        + fileProgress
                        )
                    / static_cast<double>(totalFiles);

                const int percent =
                    static_cast<int>(
                        overallProgress * 100.0
                        );

                emit progressChanged(percent);
            },
            [this]()
            {
                return waitIfPaused();
            }
            );

        if (!success) {
            QFile::remove(temporaryFilePath);

            if (stopRequested.load()) {
                emit stopped();
            } else {
                emit finished(false);
            }

            return;
        }

        if (overwrite && QFile::exists(outputFilePath)) {
            if (!QFile::remove(outputFilePath)) {
                QFile::remove(temporaryFilePath);
                emit finished(false);
                return;
            }
        }

        if (!QFile::rename(
                temporaryFilePath,
                outputFilePath)) {

            QFile::remove(temporaryFilePath);
            emit finished(false);
            return;
        }

        if (deleteInputFiles) {
            if (!QFile::remove(inputPath)) {
                emit finished(false);
                return;
            }
        }

        ++processedFiles;

        emit progressChanged(
            static_cast<int>(
                (
                    static_cast<double>(processedFiles)
                    / static_cast<double>(totalFiles)
                    ) * 100.0
                )
            );
    }

    emit finished(true);
}