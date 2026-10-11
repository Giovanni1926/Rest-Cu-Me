#pragma once

#include <QWidget>
#include <QTextEdit>
#include <vector>
#include "../../run/consoleline.h"

namespace rcm {

class ConsolePanel : public QWidget {
public:
    explicit ConsolePanel(QWidget* parent = nullptr);

    void refresh(const std::vector<ConsoleLine>& log);

private:
    static bool isDarkTheme();
    QTextEdit* textEdit_ = nullptr;
};

} // namespace rcm
