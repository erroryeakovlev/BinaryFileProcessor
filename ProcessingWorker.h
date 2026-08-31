#ifndef PROCESSINGWORKER_H
#define PROCESSINGWORKER_H

#include <QObject>
#include <QStringList>

#include <atomic>
#include <condition_variable>
#include <mutex>

class ProcessingWorker : public QObject
{
    Q_OBJECT

public:
    explicit ProcessingWorker(QObject *parent = nullptr);

    void requestStop();
    void requestPause();
    void requestResume();

public slots:
    void processFiles(
        const QStringList& inputPaths,
        const QString& outputDirectory,
        quint64 xorValue,
        bool overwrite,
        bool deleteInputFiles
        );

signals:
    void finished(bool success);
    void progressChanged(int percent);
    void stopped();

    void paused();
    void resumed();

private:
    bool waitIfPaused();

    std::atomic_bool stopRequested{false};
    std::atomic_bool pauseRequested{false};

    std::mutex pauseMutex;
    std::condition_variable pauseCondition;
};

#endif // PROCESSINGWORKER_H