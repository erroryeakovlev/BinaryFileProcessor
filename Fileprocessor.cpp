#include "FileProcessor.h"

#include <QFile>

#include <array>

bool FileProcessor::processFile(
    const QString& inputPath,
    const QString& outputPath,
    quint64 xorValue,
    const std::function<void(qint64, qint64)>& progressCallback,
    const std::function<bool()>& shouldContinue)
{
    QFile inputFile(inputPath);

    if (!inputFile.open(QIODevice::ReadOnly)) {
        return false;
    }

    QFile outputFile(outputPath);

    if (!outputFile.open(QIODevice::WriteOnly)) {
        return false;
    }

    const std::array<unsigned char, 8> key = {
        static_cast<unsigned char>((xorValue >> 56) & 0xFF),
        static_cast<unsigned char>((xorValue >> 48) & 0xFF),
        static_cast<unsigned char>((xorValue >> 40) & 0xFF),
        static_cast<unsigned char>((xorValue >> 32) & 0xFF),
        static_cast<unsigned char>((xorValue >> 24) & 0xFF),
        static_cast<unsigned char>((xorValue >> 16) & 0xFF),
        static_cast<unsigned char>((xorValue >> 8) & 0xFF),
        static_cast<unsigned char>(xorValue & 0xFF)
    };

    constexpr qint64 chunkSize = 1024 * 1024;

    const qint64 totalBytes = inputFile.size();
    qint64 processedBytes = 0;

    if (progressCallback) {
        progressCallback(0, totalBytes);
    }

    while (!inputFile.atEnd()) {
        if (shouldContinue && !shouldContinue()) {
            return false;
        }

        QByteArray buffer = inputFile.read(chunkSize);

        if (buffer.isEmpty() && !inputFile.atEnd()) {
            return false;
        }

        for (qint64 i = 0; i < buffer.size(); ++i) {
            const qint64 filePosition = processedBytes + i;

            buffer[i] = static_cast<char>(
                static_cast<unsigned char>(buffer[i])
                ^ key[filePosition % 8]
                );
        }

        if (outputFile.write(buffer) != buffer.size()) {
            return false;
        }

        processedBytes += buffer.size();

        if (progressCallback) {
            progressCallback(processedBytes, totalBytes);
        }

        if (shouldContinue && !shouldContinue()) {
            return false;
        }
    }

    return true;
}