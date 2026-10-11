#pragma once

#include <QString>
#include <QByteArray>
#include <functional>
#include "../util/str.h"
#include "../model/restcalldef.h"

namespace rcm {

class RequestBodyBuilder {
public:
    struct BuiltBody {
        QString contentType;
        QByteArray body;
        QString snapshot;
    };

    static BuiltBody build(const RestCallDef& call,
                           const std::function<QString(const QString&)>& substitute);

    static Json substituteJson(const Json& element,
                               const std::function<QString(const QString&)>& substitute);

private:
    static BuiltBody buildText(const QString& text);
    static BuiltBody buildJson(const Json& body,
                               const std::function<QString(const QString&)>& substitute);
    static BuiltBody asText(const Json& body,
                            const std::function<QString(const QString&)>& substitute);
    static BuiltBody encodeForm(const Json& body,
                                const std::function<QString(const QString&)>& substitute);
    static BuiltBody buildMultipart(const RestCallDef& call,
                                    const std::function<QString(const QString&)>& substitute);
    static QByteArray headBytes(const QString& boundary, const QString& name,
                                const QString& fileName, const QString& contentType);
    static QString quoted(const QString& value);
    static QString abbreviate(const QString& value);
};

} // namespace rcm
