#include "Fileprocessor.h"

#include <QDebug>
#include <QFile>
#include <QTemporaryDir>

#include <array>

int main()
{
    QTemporaryDir tempDir;

    if (!tempDir.isValid()) {
        qCritical() << "Failed to create temporary directory";
        return 1;
    }

    const QString inputPath =
        tempDir.filePath("input.bin");

    const QString outputPath =
        tempDir.filePath("output.bin");

    constexpr qint64 dataSize =
        3 * 1024 * 1024 + 123;

    QByteArray inputData;
    inputData.resize(dataSize);

    for (qint64 i = 0; i < dataSize; ++i) {
        inputData[i] = static_cast<char>(
            (i * 37 + 11) & 0xFF
            );
    }

    QFile inputFile(inputPath);

    if (!inputFile.open(QIODevice::WriteOnly)) {
        qCritical() << "Failed to create input file";
        return 1;
    }

    if (inputFile.write(inputData) != inputData.size()) {
        qCritical() << "Failed to write input file";
        return 1;
    }

    inputFile.close();

    constexpr quint64 xorValue =
        0x1234567890ABCDEFULL;

    FileProcessor processor;

    const bool success =
        processor.processFile(
            inputPath,
            outputPath,
            xorValue,
            [](qint64, qint64)
            {
            },
            []()
            {
                return true;
            }
            );

    if (!success) {
        qCritical() << "FileProcessor::processFile failed";
        return 1;
    }

    QFile outputFile(outputPath);

    if (!outputFile.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open output file";
        return 1;
    }

    const QByteArray outputData =
        outputFile.readAll();

    if (outputData.size() != inputData.size()) {
        qCritical()
            << "Output size mismatch:"
            << outputData.size()
            << "expected:"
            << inputData.size();

        return 1;
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

    for (qint64 i = 0; i < dataSize; ++i) {
        const unsigned char original =
            static_cast<unsigned char>(inputData[i]);

        const unsigned char actual =
            static_cast<unsigned char>(outputData[i]);

        const unsigned char expected =
            original ^ key[i % 8];

        if (actual != expected) {
            qCritical()
                << "XOR mismatch at byte"
                << i
                << "actual:"
                << actual
                << "expected:"
                << expected;

            return 1;
        }
    }

    qDebug() << "FileProcessor smoke test passed";
    return 0;
}
