#pragma once

#include <QString>

namespace rcm {

struct VariableDecl {
    QString name;
    QString value;

    VariableDecl() = default;
    VariableDecl(const QString& name, const QString& value) : name(name), value(value) {}

    QString asString() const { return value; }
};

} // namespace rcm
