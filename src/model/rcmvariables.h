#pragma once

#include <QString>
#include <vector>
#include "variabledecl.h"

namespace rcm {

class RcmVariables {
public:
    RcmVariables() = default;
    explicit RcmVariables(const std::vector<VariableDecl>& variables) : variables_(variables) {}

    const std::vector<VariableDecl>& variables() const { return variables_; }

    QString resolve(const QString& varName) const {
        for (const auto& v : variables_) {
            if (v.name == varName) return v.asString();
        }
        throw std::invalid_argument(("Undefined variable: " + varName.toStdString()));
    }

    bool hasVariable(const QString& name) const {
        for (const auto& v : variables_) {
            if (v.name == name) return true;
        }
        return false;
    }

private:
    std::vector<VariableDecl> variables_;
};

} // namespace rcm
