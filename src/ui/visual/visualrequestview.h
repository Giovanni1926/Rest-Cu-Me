#pragma once

#include <QWidget>
#include <QLabel>
#include <QStackedWidget>
#include <QPlainTextEdit>
#include "../forms/restrequestform.h"
#include "../forms/soaprequestform.h"
#include "../../flow/flowgraph.h"
#include "../../http/restclient.h"

namespace rcm {

class VisualRequestView : public QWidget {
public:
    explicit VisualRequestView(QWidget* parent = nullptr);

    void prefillFrom(const FlowNode& node, const FlowGraph& graph,
                     const QStringList& availableVariables);
    void clear();

private:
    void execute();
    void showResult(const CallResult& result);
    void showMessage(const QString& message);

    QLabel* hintLabel_ = nullptr;
    QStackedWidget* stack_ = nullptr;
    RestRequestForm* restForm_ = nullptr;
    SoapRequestForm* soapForm_ = nullptr;

    QWidget* responseWidget_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QPlainTextEdit* headersView_ = nullptr;
    QPlainTextEdit* bodyView_ = nullptr;

    bool isSoap_ = false;
    FlowNode selectedNode_;
    const FlowGraph* graph_ = nullptr;
    QString label_;
};

} // namespace rcm
