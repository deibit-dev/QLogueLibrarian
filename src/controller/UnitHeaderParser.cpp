#include "UnitHeaderParser.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

namespace qlogue {

// ─── Public interface ───────────────────────────────────────────────────────

UnitInfo UnitHeaderParser::parse(const QString &filePath)
{
    UnitInfo info;
    info.filePath = filePath;
    info.fileName = QFileInfo(filePath).fileName();

    // ── Extract manifest.json from the ZIP via unzip -p ─────────────────
    QProcess proc;
    proc.setProgram(QStringLiteral("unzip"));
    proc.setArguments({QStringLiteral("-p"), filePath, QStringLiteral("*/manifest.json")});
    proc.start();

    if (!proc.waitForFinished(5000) || proc.exitCode() != 0) {
        info.isValid = false;
        return info;
    }

    const QByteArray jsonData = proc.readAllStandardOutput();
    if (jsonData.isEmpty()) {
        info.isValid = false;
        return info;
    }

    // ── Parse JSON ──────────────────────────────────────────────────────
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonData, &err);
    if (doc.isNull()) {
        info.isValid = false;
        return info;
    }

    const QJsonObject header = doc.object()
                                   .value(QStringLiteral("header")).toObject();
    if (header.isEmpty()) {
        info.isValid = false;
        return info;
    }

    // ── Populate UnitInfo ───────────────────────────────────────────────
    info.platform  = header.value(QStringLiteral("platform")).toString();
    info.module    = header.value(QStringLiteral("module")).toString();
    info.api       = header.value(QStringLiteral("api")).toString();
    info.devId     = header.value(QStringLiteral("dev_id")).toInt();
    info.prgId     = header.value(QStringLiteral("prg_id")).toInt();
    info.version   = header.value(QStringLiteral("version")).toString();
    info.name      = header.value(QStringLiteral("name")).toString();
    info.numParams = header.value(QStringLiteral("num_param")).toInt();

    // params is an array of arrays: [["name", min, max, "type"], …]
    const QJsonArray paramsArr = header.value(QStringLiteral("params")).toArray();
    for (const QJsonValue &v : paramsArr) {
        const QJsonArray pa = v.toArray();
        if (pa.size() < 4) continue;

        UnitParam p;
        p.name = pa.at(0).toString();
        p.min  = pa.at(1).toInt();
        p.max  = pa.at(2).toInt();
        p.type = pa.at(3).toString();
        info.params.append(p);
    }

    info.isValid = true;
    return info;
}

} // namespace qlogue
