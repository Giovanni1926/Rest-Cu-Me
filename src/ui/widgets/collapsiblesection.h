#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QEvent>

namespace rcm {

class CollapsibleSection : public QWidget {
public:
    CollapsibleSection(const QString& title, QWidget* content, QWidget* parent = nullptr)
        : QWidget(parent), content_(content), title_(title) {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(4, 0, 4, 0);
        layout->setSpacing(4);

        headerLabel_ = new QLabel(this);
        headerLabel_->setCursor(Qt::PointingHandCursor);
        headerLabel_->setTextInteractionFlags(Qt::NoTextInteraction);
        headerLabel_->installEventFilter(this);

        QWidget* header = new QWidget(this);
        QVBoxLayout* headerLayout = new QVBoxLayout(header);
        headerLayout->setContentsMargins(0, 0, 0, 0);
        headerLayout->addWidget(headerLabel_);
        header->setCursor(Qt::PointingHandCursor);
        header->installEventFilter(this);

        layout->addWidget(header);
        layout->addWidget(content_);

        setExpanded(false);
        setAlignmentX();
    }

    void setExpanded(bool expanded) {
        expanded_ = expanded;
        content_->setVisible(expanded);
        updateLabel();
    }

    bool isExpanded() const { return expanded_; }

protected:
    bool eventFilter(QObject* obj, QEvent* event) override {
        if (event->type() == QEvent::MouseButtonRelease) {
            if (obj == headerLabel_ || obj == headerLabel_->parentWidget()) {
                setExpanded(!expanded_);
                return true;
            }
        }
        return QWidget::eventFilter(obj, event);
    }

private:
    void updateLabel() {
        QFont f = headerLabel_->font();
        f.setBold(true);
        headerLabel_->setFont(f);
        headerLabel_->setText((expanded_ ? QStringLiteral("\u25be  ") : QStringLiteral("\u25b8  ")) +
                              title_);
    }

    void setAlignmentX() {}

    QLabel* headerLabel_ = nullptr;
    QWidget* content_ = nullptr;
    QString title_;
    bool expanded_ = false;
};

} // namespace rcm
