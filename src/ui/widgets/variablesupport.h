#pragma once

#include <QObject>
#include <QPushButton>
#include <functional>
#include <QStringList>

class QPlainTextEdit;
class QLineEdit;
class QTableWidget;
class QWidget;

namespace rcm {

class VariableSupport : public QObject {
public:
    explicit VariableSupport(const std::function<QStringList()>& variables,
                             QObject* parent = nullptr);

    QPushButton* insertButton(QWidget* parent);
    void installPopup(QPlainTextEdit* editor);

private:
    void showChooser(QWidget* anchor, QPlainTextEdit* editor);
    void insertIntoFocused(const QString& token);

    std::function<QStringList()> variables_;
};

} // namespace rcm
