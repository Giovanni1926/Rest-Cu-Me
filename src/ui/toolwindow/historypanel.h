#pragma once

#include <QWidget>
#include <vector>
#include "../../model/callrecord.h"

namespace rcm {

class HistoryPanel : public QWidget {
public:
    explicit HistoryPanel(QWidget* parent = nullptr);

    void refresh(const std::vector<CallRecord>& history);

private:
    void clearHistory();

    QWidget* listPanel_ = nullptr;
};

} // namespace rcm
