#pragma once

#include <QString>
#include <vector>
#include "../model/callrecord.h"

namespace rcm {

class HistoryManager {
public:
    explicit HistoryManager(const QString& basePath);

    QString saveRecord(const CallRecord& record);
    QString generateId() const;
    std::vector<CallRecord> loadAll();
    void clearAll();

private:
    QString historyDir() const;
    void ensureHistoryDir() const;

    QString basePath_;
};

} // namespace rcm
