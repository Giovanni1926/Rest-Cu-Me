#include "rcmflowrunservice.h"
#include "flowplanner.h"
#include "jsonpathextractor.h"
#include "soapcallbuilder.h"
#include "../model/restcallparser.h"
#include "../run/rcmrunservice.h"

#include <QCoreApplication>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <set>

namespace rcm {

static const int MAX_VAR_DEPTH = 20;

RcmFlowRunService& RcmFlowRunService::instance() {
    static RcmFlowRunService s;
    return s;
}

int RcmFlowRunService::addListener(const std::function<void()>& listener) {
    const int id = nextListenerId_++;
    listeners_.emplace_back(id, listener);
    return id;
}

void RcmFlowRunService::removeListener(int id) {
    listeners_.erase(std::remove_if(listeners_.begin(), listeners_.end(),
                                    [&](const std::pair<int, std::function<void()>>& p) {
                                        return p.first == id;
                                    }),
                     listeners_.end());
}

void RcmFlowRunService::runAll(const FlowGraph& graph, const QString& title) {
    run(graph, QString(), title);
}

void RcmFlowRunService::runFrom(const FlowGraph& graph, const QString& startNodeId,
                                const QString& title) {
    run(graph, startNodeId, title);
}

QString RcmFlowRunService::firstValue(const std::vector<VariableDecl>& context,
                                      const QString& name) {
    for (const auto& v : context) {
        if (v.name == name) return v.value;
    }
    return QString();
}

QString RcmFlowRunService::normalize(const Json& value) {
    if (value.is_null()) return QString();
    if (value.is_string()) return s2q(value.get<std::string>());
    return s2q(value.dump());
}

QString RcmFlowRunService::resolveDeep(const QString& value,
                                       const std::vector<VariableDecl>& context,
                                       std::set<QString>& seen) {
    if (value.isNull()) return QString();
    if (static_cast<int>(seen.size()) > MAX_VAR_DEPTH) return value;
    QString result;
    int i = 0;
    while (i < value.size()) {
        if (value.at(i) == QLatin1Char('$') && i + 1 < value.size() &&
            value.at(i + 1) == QLatin1Char('{')) {
            const int close = value.indexOf(QLatin1Char('}'), i + 2);
            if (close > 0) {
                const QString name = value.mid(i + 2, close - i - 2);
                const QString match = firstValue(context, name);
                if (match.isNull()) {
                    result += QStringLiteral("${") + name + QStringLiteral("}");
                } else if (seen.count(name)) {
                    result += match;
                } else {
                    seen.insert(name);
                    result += resolveDeep(match, context, seen);
                    seen.erase(name);
                }
                i = close + 1;
                continue;
            }
        }
        result += value.at(i);
        ++i;
    }
    return result;
}

std::vector<VariableDecl> RcmFlowRunService::resolveContext(
    const std::vector<VariableDecl>& raw) {
    std::vector<VariableDecl> result;
    for (const auto& v : raw) {
        std::set<QString> seen;
        result.emplace_back(v.name, resolveDeep(v.value, raw, seen));
    }
    return result;
}

VarMap RcmFlowRunService::resolveVariableNode(const FlowNode& node,
                                              const std::vector<VariableDecl>& context) {
    std::vector<VariableDecl> local = context;
    VarMap resolved;
    for (const auto& kv : node.vars) {
        if (kv.first.trimmed().isEmpty()) continue;
        std::set<QString> seen;
        const QString value = resolveDeep(kv.second, local, seen);
        resolved.emplace_back(kv.first, value);
        local.insert(local.begin(), VariableDecl(kv.first, value));
    }
    return resolved;
}

std::vector<VariableDecl> RcmFlowRunService::buildRawContext(
    const FlowNode& node, const FlowGraph& graph,
    const std::map<QString, std::vector<FlowEdge>>& incoming,
    const std::map<QString, CallResult>& runResults,
    const std::map<QString, VarMap>& variableOutputs) {
    std::vector<VariableDecl> context;

    auto incomingIt = incoming.find(node.id);
    if (incomingIt != incoming.end()) {
        for (const auto& edge : incomingIt->second) {
            const FlowNode* source = graph.nodeById(edge.from);
            if (!source) continue;
            if (source->isVariableNode()) {
                auto vo = variableOutputs.find(edge.from);
                if (vo == variableOutputs.end()) continue;
                for (const auto& kv : vo->second) {
                    context.emplace_back(kv.first, kv.second);
                }
            } else {
                auto rr = runResults.find(edge.from);
                if (rr == runResults.end() || !edge.outputs) continue;
                for (const auto& output : *edge.outputs) {
                    if (output.name.trimmed().isEmpty()) continue;
                    context.emplace_back(output.name,
                                         JsonPathExtractor::extract(output.expr, rr->second));
                }
            }
        }
    }

    if (const FlowGroup* group = graph.groupOf(node.id)) {
        for (const auto& kv : group->variables) {
            context.emplace_back(kv.first, kv.second);
        }
    }

    for (auto it = graph.variables().begin(); it != graph.variables().end(); ++it) {
        context.emplace_back(s2q(it.key()), normalize(it.value()));
    }

    return context;
}

void RcmFlowRunService::scanText(const QString& text,
                                 const std::vector<VariableDecl>& context,
                                 std::set<QString>& unresolved, std::set<QString>& visiting) {
    int i = 0;
    while (i < text.size()) {
        if (text.at(i) == QLatin1Char('$') && i + 1 < text.size() &&
            text.at(i + 1) == QLatin1Char('{')) {
            const int close = text.indexOf(QLatin1Char('}'), i + 2);
            if (close > 0) {
                const QString name = text.mid(i + 2, close - i - 2);
                const QString value = firstValue(context, name);
                if (value.isNull()) {
                    unresolved.insert(name);
                } else if (!visiting.count(name) &&
                           static_cast<int>(visiting.size()) < MAX_VAR_DEPTH) {
                    visiting.insert(name);
                    scanText(value, context, unresolved, visiting);
                    visiting.erase(name);
                }
                i = close + 1;
                continue;
            }
        }
        ++i;
    }
}

void RcmFlowRunService::scanUnresolved(const Json& element,
                                       const std::vector<VariableDecl>& context,
                                       std::set<QString>& unresolved,
                                       std::set<QString>& visiting) {
    if (element.is_null()) return;
    if (element.is_array()) {
        for (const auto& item : element) {
            scanUnresolved(item, context, unresolved, visiting);
        }
    } else if (element.is_object()) {
        for (auto it = element.begin(); it != element.end(); ++it) {
            scanText(s2q(it.key()), context, unresolved, visiting);
            scanUnresolved(it.value(), context, unresolved, visiting);
        }
    } else if (element.is_string()) {
        scanText(s2q(element.get<std::string>()), context, unresolved, visiting);
    }
}

std::vector<QString> RcmFlowRunService::unresolvedVariables(
    const Json& call, const std::vector<VariableDecl>& context) {
    std::set<QString> unresolved;
    std::set<QString> visiting;
    scanUnresolved(call, context, unresolved, visiting);
    return std::vector<QString>(unresolved.begin(), unresolved.end());
}

QString RcmFlowRunService::firstUnresolvedVariable(const Json& call,
                                                   const std::vector<VariableDecl>& context) {
    if (call.is_null() || !call.is_object()) return QString();
    auto bodyIt = call.find("body");
    if (bodyIt == call.end() || !bodyIt->is_string()) return QString();
    const QString body = s2q(bodyIt->get<std::string>());
    int i = 0;
    while (i < body.size()) {
        if (body.at(i) == QLatin1Char('$') && i + 1 < body.size() &&
            body.at(i + 1) == QLatin1Char('{')) {
            const int close = body.indexOf(QLatin1Char('}'), i + 2);
            if (close > 0) {
                const QString name = body.mid(i + 2, close - i - 2);
                if (firstValue(context, name).isNull()) return name;
                i = close + 1;
                continue;
            }
        }
        ++i;
    }
    return QString();
}

CallResult RcmFlowRunService::errorResult(const FlowNode& node, const QString& message) {
    QString method = QStringLiteral("GET");
    QString url;
    if (!node.call.is_null() && node.call.is_object()) {
        auto m = node.call.find("method");
        if (m != node.call.end() && m->is_string()) method = s2q(m->get<std::string>());
        auto u = node.call.find("url");
        if (u != node.call.end() && u->is_string()) url = s2q(u->get<std::string>());
    }
    return CallResult(method, url, 0, QStringLiteral("Error"), QString(), 0, false, message);
}

QString RcmFlowRunService::soapOperationOf(const FlowNode& node) {
    if (node.call.is_null() || !node.call.is_object()) return QString();
    auto soapIt = node.call.find("soap");
    if (soapIt == node.call.end() || !soapIt->is_object()) return QString();
    auto op = soapIt->find("operation");
    if (op == soapIt->end() || !op->is_string()) return QString();
    return s2q(op->get<std::string>());
}

QString RcmFlowRunService::labelOf(const FlowNode& node) {
    if (!node.title.trimmed().isEmpty()) return node.title;
    if (node.isVariableNode()) return QStringLiteral("Nodo variabili");
    if (node.isSoapNode()) {
        const QString op = soapOperationOf(node);
        if (!op.trimmed().isEmpty()) return QStringLiteral("SOAP ") + op;
        QString url;
        if (node.call.is_object()) {
            auto u = node.call.find("url");
            if (u != node.call.end() && u->is_string()) url = s2q(u->get<std::string>());
        }
        return QStringLiteral("SOAP ") + url;
    }
    QString method = QStringLiteral("GET");
    QString url;
    if (node.call.is_object()) {
        auto m = node.call.find("method");
        if (m != node.call.end() && m->is_string()) method = s2q(m->get<std::string>());
        auto u = node.call.find("url");
        if (u != node.call.end() && u->is_string()) url = s2q(u->get<std::string>());
    }
    return method + QStringLiteral(" ") + url;
}

namespace {

struct RunWorkerResult {
    std::vector<RcmFlowRunService::FlowOutputEvent> events;
    std::map<QString, CallResult> resultMap;
};

} // namespace

void RcmFlowRunService::run(const FlowGraph& graph, const QString& startNodeId,
                            const QString& title) {
    auto* watcher = new QFutureWatcher<RunWorkerResult>();
    QObject::connect(watcher, &QFutureWatcher<RunWorkerResult>::finished, watcher,
                     [this, watcher]() {
                         const RunWorkerResult wr = watcher->result();
                         RcmRunService& hub = RcmRunService::instance();
                         for (const auto& event : wr.events) {
                             if (event.isResult) {
                                 hub.appendResult(event.result, event.label);
                             } else {
                                 hub.appendLog(event.line.text, event.line.kind);
                             }
                         }
                         results_ = wr.resultMap;
                         hub.fireRefresh();
                         for (const auto& listener : listeners_) listener.second();
                         watcher->deleteLater();
                     });
    watcher->setFuture(QtConcurrent::run([graph, startNodeId, title]() {
        RunWorkerResult wr;
        std::vector<QString> planErrors;
        const std::vector<FlowNode> order =
            FlowPlanner::plan(graph, startNodeId, planErrors);

        if (!planErrors.empty()) {
            QString joined;
            for (size_t i = 0; i < planErrors.size(); ++i) {
                if (i) joined += QStringLiteral("; ");
                joined += planErrors[i];
            }
            FlowOutputEvent e;
            e.isResult = false;
            e.line = ConsoleLine(QStringLiteral("[FLOW] ") + title + QStringLiteral(" \u2014 ") + joined,
                                 ConsoleLine::INFO);
            wr.events.push_back(e);
            return wr;
        }
        if (order.empty()) {
            FlowOutputEvent e;
            e.isResult = false;
            e.line = ConsoleLine(QStringLiteral("[FLOW] ") + title +
                                     QStringLiteral(" \u2014 nessun nodo da eseguire"),
                                 ConsoleLine::INFO);
            wr.events.push_back(e);
            return wr;
        }

        std::map<QString, std::vector<FlowEdge>> incoming;
        for (const auto& edge : graph.edges()) {
            incoming[edge.to].push_back(edge);
        }

        std::map<QString, CallResult> runResults;
        std::map<QString, VarMap> variableOutputs;
        std::map<QString, std::vector<QString>> unresolvedWarnings;

        for (const auto& node : order) {
            std::vector<VariableDecl> rawContext =
                buildRawContext(node, graph, incoming, runResults, variableOutputs);

            if (node.isVariableNode()) {
                variableOutputs[node.id] = resolveVariableNode(node, rawContext);
                continue;
            }

            if (node.call.is_null()) {
                runResults[node.id] = errorResult(
                    node, QStringLiteral("Nodo REST senza definizione della chiamata"));
                continue;
            }

            const std::vector<VariableDecl> context = resolveContext(rawContext);
            Json callJson = node.isSoapNode() ? SoapCallBuilder::prepare(node.call) : node.call;

            if (node.isSoapNode()) {
                const QString unresolved = firstUnresolvedVariable(callJson, context);
                if (!unresolved.isNull()) {
                    runResults[node.id] = errorResult(
                        node, QStringLiteral("Variabile non risolta nel body SOAP: ${") +
                                  unresolved + QStringLiteral("}"));
                    continue;
                }
            }

            const std::vector<QString> unresolved =
                unresolvedVariables(callJson, context);
            if (!unresolved.empty()) {
                unresolvedWarnings[node.id] = unresolved;
            }

            std::vector<QString> parseErrors;
            auto call = RestCallParser::parseCallDefinition(callJson, parseErrors);
            if (!call) {
                QString message = parseErrors.empty()
                                      ? QStringLiteral("Definizione della chiamata non valida")
                                      : [&]() {
                                            QString joined;
                                            for (size_t i = 0; i < parseErrors.size(); ++i) {
                                                if (i) joined += QStringLiteral("; ");
                                                joined += parseErrors[i];
                                            }
                                            return joined;
                                        }();
                runResults[node.id] = errorResult(node, message);
                continue;
            }

            RestClient client{RcmVariables(context)};
            runResults[node.id] = client.execute(*call);
        }

        FlowOutputEvent headerEvent;
        headerEvent.isResult = false;
        headerEvent.line =
            ConsoleLine(QStringLiteral("[FLOW] ") + title, ConsoleLine::HEADER);
        wr.events.push_back(headerEvent);

        int failures = 0;
        for (const auto& node : order) {
            if (node.isVariableNode()) {
                auto vo = variableOutputs.find(node.id);
                int count = vo != variableOutputs.end() ? static_cast<int>(vo->second.size()) : 0;
                FlowOutputEvent e;
                e.isResult = false;
                e.line = ConsoleLine(labelOf(node) + QStringLiteral(" \u2014 ") +
                                         QString::number(count) + QStringLiteral(" variabili"),
                                     ConsoleLine::MUTED);
                wr.events.push_back(e);
                continue;
            }

            auto warnIt = unresolvedWarnings.find(node.id);
            if (warnIt != unresolvedWarnings.end()) {
                QStringList names;
                for (const auto& n : warnIt->second) names << QStringLiteral("${") + n + QStringLiteral("}");
                FlowOutputEvent e;
                e.isResult = false;
                e.line = ConsoleLine(labelOf(node) +
                                         QStringLiteral(" \u2014 variabili non risolte, inviate come testo: ") +
                                         names.join(QStringLiteral(", ")) +
                                         QStringLiteral(" (collega un nodo variabile o un arco, oppure definiscile tra le variabili globali/collection)"),
                                     ConsoleLine::ERROR);
                wr.events.push_back(e);
            }

            auto resultIt = runResults.find(node.id);
            if (resultIt == runResults.end()) continue;

            FlowOutputEvent e;
            e.isResult = true;
            e.result = resultIt->second;
            e.label = labelOf(node);
            wr.events.push_back(e);

            if (!resultIt->second.errorMessage.isEmpty() || !resultIt->second.success) {
                ++failures;
            }
        }

        FlowOutputEvent doneEvent;
        doneEvent.isResult = false;
        QString doneText = QStringLiteral("[FLOW COMPLETATO] ") + QString::number(order.size()) +
                           QStringLiteral(" chiamate");
        if (failures > 0) doneText += QStringLiteral(" \u00b7 ") + QString::number(failures) +
                                      QStringLiteral(" errori");
        doneEvent.line = ConsoleLine(doneText,
                                     failures > 0 ? ConsoleLine::ERROR : ConsoleLine::INFO);
        wr.events.push_back(doneEvent);

        wr.resultMap = runResults;
        return wr;
    }));
}

} // namespace rcm
