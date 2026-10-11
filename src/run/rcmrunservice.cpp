#include "rcmrunservice.h"

#include <QStandardPaths>
#include <QDir>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QCoreApplication>

namespace rcm {

RcmRunService& RcmRunService::instance() {
    static RcmRunService s;
    return s;
}

static QString historyBaseDir() {
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty()) base = QDir::homePath();
    return base;
}

RcmRunService::RcmRunService() : historyManager_(historyBaseDir()) {
    const auto loaded = historyManager_.loadAll();
    history_.insert(history_.end(), loaded.begin(), loaded.end());
}

int RcmRunService::addListener(const std::function<void()>& listener) {
    const int id = nextListenerId_++;
    listeners_.emplace_back(id, listener);
    return id;
}

void RcmRunService::removeListener(int id) {
    listeners_.erase(std::remove_if(listeners_.begin(), listeners_.end(),
                                    [&](const std::pair<int, std::function<void()>>& p) {
                                        return p.first == id;
                                    }),
                     listeners_.end());
}

void RcmRunService::appendLog(const QString& line, ConsoleLine::Kind kind) {
    consoleLog_.push_back(ConsoleLine(line, kind));
}

void RcmRunService::appendResult(const CallResult& result, const QString& label) {
    const bool error = !result.errorMessage.isEmpty();
    const ConsoleLine::Kind kind =
        (error || !result.success) ? ConsoleLine::ERROR : ConsoleLine::SUCCESS;
    const QString text = (label.isEmpty()) ? result.summary()
                                           : label + QStringLiteral("  \u00b7  ") + result.summary();
    consoleLog_.push_back(ConsoleLine(text, kind));

    try {
        CallRecord record(historyManager_.generateId(), result.method, result.url, label,
                          result.requestHeaders, result.requestCookies, result.requestBody,
                          result.statusCode, result.responseHeaders, result.responseCookies,
                          result.responseBody);
        historyManager_.saveRecord(record);
        history_.insert(history_.begin(), record);
    } catch (const std::exception& e) {
        consoleLog_.push_back(
            ConsoleLine(QStringLiteral("Impossibile salvare la cronologia: ") +
                            QString::fromUtf8(e.what()),
                        ConsoleLine::ERROR));
    }
}

void RcmRunService::fireRefresh() {
    for (const auto& listener : listeners_) listener.second();
}

void RcmRunService::clearHistory() {
    historyManager_.clearAll();
    history_.clear();
    fireRefresh();
}

void RcmRunService::replay(const CallRecord& record) {
    auto* watcher = new QFutureWatcher<CallResult>();
    QObject::connect(watcher, &QFutureWatcher<CallResult>::finished, watcher,
                     [this, watcher]() {
                         const CallResult r = watcher->result();
                         appendResult(r, QStringLiteral("Replay"));
                         fireRefresh();
                         watcher->deleteLater();
                     });
    watcher->setFuture(QtConcurrent::run([record]() {
        RestClient client{RcmVariables{}};
        return client.executeRaw(record.method, record.url, record.requestHeaders,
                                 record.requestCookies, record.requestBody);
    }));
}

} // namespace rcm
