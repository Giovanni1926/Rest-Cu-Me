#include "historymanager.h"
#include <QDir>
#include <QFile>
#include <QUuid>
#include <algorithm>
#include <cstdio>

namespace rcm {

HistoryManager::HistoryManager(const QString& basePath) : basePath_(basePath) {}

QString HistoryManager::historyDir() const {
    return QDir(basePath_).filePath(QStringLiteral(".rcm-history"));
}

void HistoryManager::ensureHistoryDir() const {
    QDir dir;
    if (!dir.mkpath(historyDir())) {
        std::fprintf(stderr, "Cannot create history directory: %s\n",
                     historyDir().toUtf8().constData());
    }
}

QString HistoryManager::generateId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

QString HistoryManager::saveRecord(const CallRecord& record) {
    if (record.id.isEmpty()) return QString();
    ensureHistoryDir();
    const QString fileName = record.id + QStringLiteral(".txt");
    QFile file(QDir(historyDir()).filePath(fileName));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        std::fprintf(stderr, "Cannot save history record: %s\n",
                     file.errorString().toUtf8().constData());
        return QString();
    }
    file.write(record.serialize().toUtf8());
    file.close();
    return fileName;
}

std::vector<CallRecord> HistoryManager::loadAll() {
    std::vector<CallRecord> records;
    QDir dir(historyDir());
    if (!dir.exists()) return records;

    QStringList files = dir.entryList(QStringList() << QStringLiteral("*.txt"), QDir::Files);
    std::sort(files.begin(), files.end(), std::greater<QString>());

    for (const QString& file : files) {
        QFile f(dir.filePath(file));
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            std::fprintf(stderr, "Cannot read history file: %s\n",
                         file.toUtf8().constData());
            continue;
        }
        const QString content = QString::fromUtf8(f.readAll());
        f.close();
        records.push_back(CallRecord::deserialize(content));
    }

    std::sort(records.begin(), records.end(),
              [](const CallRecord& a, const CallRecord& b) {
                  return a.timestamp.compare(b.timestamp) > 0;
              });

    return records;
}

void HistoryManager::clearAll() {
    QDir dir(historyDir());
    if (!dir.exists()) return;
    const QStringList files =
        dir.entryList(QStringList() << QStringLiteral("*.txt"), QDir::Files);
    for (const QString& file : files) {
        dir.remove(file);
    }
}

} // namespace rcm
