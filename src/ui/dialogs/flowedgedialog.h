#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QLabel>
#include <optional>
#include "../../flow/flowgraph.h"
#include "../../http/restclient.h"

namespace rcm {

class FlowEdgeDialog : public QDialog {
public:
    FlowEdgeDialog(const FlowNode& source, const QString& targetLabel,
                   const std::vector<FlowOutput>& existingOutputs, const CallResult* lastResult,
                   bool isNew, QWidget* parent = nullptr);

    std::optional<std::vector<FlowOutput>> getEdgeOutputs() const;

private:
    bool variableSource_;
    QTableWidget* table_ = nullptr;
    QLabel* previewLabel_ = nullptr;
};

} // namespace rcm
