#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include "consolepanel.h"
#include "historypanel.h"
#include "../../run/rcmrunservice.h"

namespace rcm {

class RestToolWindow : public QWidget {
public:
    explicit RestToolWindow(QWidget* parent = nullptr) : QWidget(parent) {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        tabs_ = new QTabWidget(this);
        console_ = new ConsolePanel(tabs_);
        history_ = new HistoryPanel(tabs_);
        tabs_->addTab(console_, QStringLiteral("Console"));
        tabs_->addTab(history_, QStringLiteral("Cronologia"));
        tabs_->setCurrentWidget(history_);
        layout->addWidget(tabs_);

        listenerId_ = RcmRunService::instance().addListener([this]() { refresh(); });
        refresh();
    }

    ~RestToolWindow() override {
        RcmRunService::instance().removeListener(listenerId_);
    }

    void showCronologia() { tabs_->setCurrentWidget(history_); }

private:
    void refresh() {
        console_->refresh(RcmRunService::instance().getConsoleLog());
        history_->refresh(RcmRunService::instance().getHistory());
    }

    QTabWidget* tabs_ = nullptr;
    ConsolePanel* console_ = nullptr;
    HistoryPanel* history_ = nullptr;
    int listenerId_ = -1;
};

} // namespace rcm
