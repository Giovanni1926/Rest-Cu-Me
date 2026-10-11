#include "visualrequestview.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QFontDatabase>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include "../widgets/appicons.h"
#include "../../model/restcallparser.h"
#include "../../flow/soapcallbuilder.h"
#include "../../flow/rcmflowrunservice.h"
#include "../../run/rcmrunservice.h"

namespace rcm {

VisualRequestView::VisualRequestView(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    hintLabel_ = new QLabel(this);
    hintLabel_->setWordWrap(true);
    hintLabel_->setStyleSheet(QStringLiteral("color: gray;"));
    root->addWidget(hintLabel_);

    restForm_ = new RestRequestForm({}, this);
    soapForm_ = new SoapRequestForm({}, this);
    stack_ = new QStackedWidget(this);
    stack_->addWidget(restForm_);
    stack_->addWidget(soapForm_);

    QScrollArea* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setWidget(stack_);
    scroll->setFrameShape(QFrame::NoFrame);
    root->addWidget(scroll, 1);

    QHBoxLayout* executeRow = new QHBoxLayout();
    executeRow->addStretch();
    QPushButton* executeButton = new QPushButton(QStringLiteral("Esegui"), this);
    executeButton->setIcon(historyIcon(palette().color(QPalette::ButtonText)));
    executeRow->addWidget(executeButton);
    root->addLayout(executeRow);

    responseWidget_ = new QWidget(this);
    QVBoxLayout* responseLayout = new QVBoxLayout(responseWidget_);
    responseLayout->setContentsMargins(0, 0, 0, 0);
    statusLabel_ = new QLabel(responseWidget_);
    statusLabel_->setWordWrap(true);
    responseLayout->addWidget(statusLabel_);

    QFont titleFont = statusLabel_->font();
    titleFont.setBold(true);

    QLabel* headersTitle = new QLabel(QStringLiteral("Header di risposta"), responseWidget_);
    headersTitle->setFont(titleFont);
    responseLayout->addWidget(headersTitle);

    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSize(9);

    headersView_ = new QPlainTextEdit(responseWidget_);
    headersView_->setReadOnly(true);
    headersView_->setFont(mono);
    headersView_->setMaximumHeight(90);
    responseLayout->addWidget(headersView_);

    QLabel* bodyTitle = new QLabel(QStringLiteral("Corpo"), responseWidget_);
    bodyTitle->setFont(titleFont);
    responseLayout->addWidget(bodyTitle);

    bodyView_ = new QPlainTextEdit(responseWidget_);
    bodyView_->setReadOnly(true);
    bodyView_->setFont(mono);
    responseLayout->addWidget(bodyView_);

    responseWidget_->setVisible(false);
    root->addWidget(responseWidget_, 1);

    connect(executeButton, &QPushButton::clicked, this, [this]() { execute(); });

    showMessage(QStringLiteral("Seleziona un nodo REST o SOAP nel grafo per precompilare la richiesta."));
}

void VisualRequestView::clear() {
    showMessage(QStringLiteral("Seleziona un nodo REST o SOAP nel grafo per precompilare la richiesta."));
    responseWidget_->setVisible(false);
    graph_ = nullptr;
    isSoap_ = false;
}

void VisualRequestView::prefillFrom(const FlowNode& node, const FlowGraph& graph,
                                    const QStringList& availableVariables) {
    graph_ = &graph;
    selectedNode_ = node;
    label_ = node.title.isEmpty() ? node.id : node.title;

    if (node.isVariableNode()) {
        showMessage(QStringLiteral("Il nodo variabili non contiene una chiamata: seleziona un nodo REST o SOAP."));
        responseWidget_->setVisible(false);
        return;
    }

    if (node.isSoapNode()) {
        isSoap_ = true;
        soapForm_->setAvailableVariables(availableVariables);
        soapForm_->loadFrom(node.call);
        stack_->setCurrentWidget(soapForm_);
    } else {
        isSoap_ = false;
        restForm_->setAvailableVariables(availableVariables);
        restForm_->loadFrom(node.call);
        stack_->setCurrentWidget(restForm_);
    }

    hintLabel_->setText(QString());
    responseWidget_->setVisible(false);
}

void VisualRequestView::execute() {
    Json call;
    QString error;
    if (isSoap_) {
        error = soapForm_->validationError();
        if (!error.isEmpty()) {
            showMessage(error);
            return;
        }
        call = SoapCallBuilder::prepare(soapForm_->getCall());
    } else {
        error = restForm_->validationError();
        if (!error.isEmpty()) {
            showMessage(error);
            return;
        }
        call = restForm_->getCall();
    }

    std::vector<QString> errors;
    auto def = RestCallParser::parseCallDefinition(call, errors);
    if (!def) {
        showMessage(errors.empty() ? QStringLiteral("Definizione della richiesta non valida")
                                   : errors.front());
        return;
    }

    std::vector<VariableDecl> context;
    if (graph_) {
        std::vector<VariableDecl> raw =
            RcmFlowRunService::buildRawContext(selectedNode_, *graph_, {}, {}, {});
        context = RcmFlowRunService::resolveContext(raw);
    }

    const QString label = label_.isEmpty() ? QStringLiteral("Visual") : label_;
    auto* watcher = new QFutureWatcher<CallResult>();
    QObject::connect(watcher, &QFutureWatcher<CallResult>::finished, watcher,
                     [this, watcher, label]() {
                         const CallResult r = watcher->result();
                         showResult(r);
                         RcmRunService::instance().appendResult(
                             r, QStringLiteral("Visual \u00b7 ") + label);
                         RcmRunService::instance().fireRefresh();
                         watcher->deleteLater();
                     });
    watcher->setFuture(QtConcurrent::run([def, context]() {
        RestClient client{RcmVariables(context)};
        return client.execute(*def);
    }));
}

void VisualRequestView::showResult(const CallResult& result) {
    responseWidget_->setVisible(true);
    if (!result.errorMessage.isEmpty()) {
        statusLabel_->setText(QStringLiteral("Errore: ") + result.errorMessage);
        statusLabel_->setStyleSheet(QStringLiteral("color: #C62828; font-weight: bold;"));
    } else {
        const bool ok = result.statusCode >= 200 && result.statusCode < 300;
        statusLabel_->setText(QString::number(result.statusCode) + QStringLiteral(" ") +
                              result.statusText + QStringLiteral(" (") +
                              QString::number(result.latencyMs) + QStringLiteral("ms)"));
        statusLabel_->setStyleSheet(
            QStringLiteral("color: %1; font-weight: bold;").arg(ok ? QStringLiteral("#2E7D32")
                                                                    : QStringLiteral("#C62828")));
    }
    QString headers;
    for (const auto& kv : result.responseHeaders) {
        headers += kv.first + QStringLiteral(": ") + kv.second + QStringLiteral("\n");
    }
    headersView_->setPlainText(headers);

    QString body = result.responseBody;
    try {
        body = s2q(Json::parse(q2s(body)).dump(2));
    } catch (...) {
    }
    bodyView_->setPlainText(body);
}

void VisualRequestView::showMessage(const QString& message) {
    hintLabel_->setText(message);
    responseWidget_->setVisible(false);
}

} // namespace rcm
