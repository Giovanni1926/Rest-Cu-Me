#pragma once

#include <QString>
#include <vector>
#include <functional>
#include "consoleline.h"
#include "../http/restclient.h"
#include "../model/callrecord.h"
#include "../http/historymanager.h"

namespace rcm {

class RcmRunService {
public:
    static RcmRunService& instance();

    std::vector<ConsoleLine> getConsoleLog() const { return consoleLog_; }
    std::vector<CallRecord> getHistory() const { return history_; }

    int addListener(const std::function<void()>& listener);
    void removeListener(int id);

    void appendLog(const QString& line) { appendLog(line, ConsoleLine::INFO); }
    void appendLog(const QString& line, ConsoleLine::Kind kind);
    void appendResult(const CallResult& result, const QString& label);
    void fireRefresh();
    void clearHistory();
    void replay(const CallRecord& record);

private:
    RcmRunService();

    std::vector<ConsoleLine> consoleLog_;
    std::vector<CallRecord> history_;
    std::vector<std::pair<int, std::function<void()>>> listeners_;
    HistoryManager historyManager_;
    int nextListenerId_ = 1;
};

} // namespace rcm
