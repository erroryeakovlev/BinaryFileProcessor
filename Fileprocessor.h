#ifndef FILEPROCESSOR_H
#define FILEPROCESSOR_H

#include <QString>

#include <functional>

class FileProcessor
{
public:
    bool processFile(
        const QString& inputPath,
        const QString& outputPath,
        quint64 xorValue,
        const std::function<void(qint64, qint64)>& progressCallback,
        const std::function<bool()>& shouldContinue
        );
};

#endif // FILEPROCESSOR_H