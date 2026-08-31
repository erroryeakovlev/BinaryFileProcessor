#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDateTime>
#include <QHash>
#include <QMainWindow>
#include <QString>
#include <QStringList>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class ProcessingWorker;
class QThread;
class QTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void startProcessing(
        const QStringList& inputPaths,
        const QString& outputDirectory,
        quint64 xorValue,
        bool overwrite,
        bool deleteInputFiles
        );

private:
    struct FileState
    {
        qint64 size;
        QDateTime lastModified;
    };

    bool validateInput();
    void scanAndStartProcessing();

    Ui::MainWindow *ui;

    QThread* processingThread = nullptr;
    ProcessingWorker* processingWorker = nullptr;
    QTimer* processingTimer = nullptr;

    bool currentRunTimerMode = false;

    QHash<QString, FileState> processedInputFiles;
    QHash<QString, FileState> currentProcessingFileStates;
};

#endif // MAINWINDOW_H