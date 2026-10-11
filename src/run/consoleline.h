#pragma once

#include <QString>

namespace rcm {

struct ConsoleLine {
    enum Kind { INFO, SUCCESS, ERROR, HEADER, MUTED };

    QString text;
    Kind kind = INFO;

    ConsoleLine() = default;
    ConsoleLine(const QString& text, Kind kind) : text(text), kind(kind) {}

    static ConsoleLine info(const QString& t) { return ConsoleLine(t, INFO); }
    static ConsoleLine success(const QString& t) { return ConsoleLine(t, SUCCESS); }
    static ConsoleLine error(const QString& t) { return ConsoleLine(t, ERROR); }
    static ConsoleLine header(const QString& t) { return ConsoleLine(t, HEADER); }
    static ConsoleLine muted(const QString& t) { return ConsoleLine(t, MUTED); }
};

} // namespace rcm
