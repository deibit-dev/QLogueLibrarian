#include "UnitHeaderParser.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "miniz.h"

namespace qlogue {

// ─── Public interface ───────────────────────────────────────────────────────

UnitInfo UnitHeaderParser::parse(const QString &filePath)
{
    UnitInfo info;
    info.setFilePath(filePath);
    info.setFileName(QFileInfo(filePath).fileName());

    // ── Open ZIP archive via miniz ────────────────────────────────────
    const QByteArray pathUtf8 = filePath.toUtf8();
    mz_zip_archive zip;
    mz_zip_zero_struct(&zip);
    if (!mz_zip_reader_init_file(&zip, pathUtf8.constData(), 0)) {
        info.setValid(false);
        return info;
    }

    // ── Find an entry whose name ends with "/manifest.json" or "manifest.json" ──
    const mz_uint fileCount = mz_zip_reader_get_num_files(&zip);
    char entryName[512];
    QByteArray manifestBytes;
    bool found = false;

    for (mz_uint i = 0; i < fileCount && !found; ++i) {
        const mz_uint n = mz_zip_reader_get_filename(&zip, i, entryName,
                                                     sizeof(entryName));
        if (n == 0 || n >= sizeof(entryName))
            continue;

        // n includes the trailing NUL; fromUtf8 stops at it.
        const QString name = QString::fromUtf8(entryName);
        const bool isManifest = (name == QLatin1String("manifest.json"))
                              || name.endsWith(QLatin1String("/manifest.json"));
        if (!isManifest)
            continue;

        size_t extractedSize = 0;
        void *data = mz_zip_reader_extract_file_to_heap(&zip, entryName,
                                                        &extractedSize, 0);
        if (!data) {
            // Try by file index
            data = mz_zip_reader_extract_to_heap(&zip, i, &extractedSize, 0);
        }
        if (!data) {
            mz_zip_reader_end(&zip);
            info.setValid(false);
            return info;
        }

        // Build QByteArray from extracted data
        manifestBytes = QByteArray(static_cast<const char *>(data),
                                   static_cast<int>(extractedSize));
        mz_free(data);
        found = true;
    }

    mz_zip_reader_end(&zip);

    if (!found || manifestBytes.isEmpty()) {
        info.setValid(false);
        return info;
    }

    // ── Parse JSON ──────────────────────────────────────────────────────
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(manifestBytes, &err);
    if (doc.isNull()) {
        info.setValid(false);
        return info;
    }

    const QJsonObject header = doc.object()
                                   .value(QStringLiteral("header")).toObject();
    if (header.isEmpty()) {
        info.setValid(false);
        return info;
    }

    // ── Populate UnitInfo ───────────────────────────────────────────────
    info.setPlatform(header.value(QStringLiteral("platform")).toString());
    info.setModule(header.value(QStringLiteral("module")).toString());
    info.setApi(header.value(QStringLiteral("api")).toString());
    info.setDevId(header.value(QStringLiteral("dev_id")).toInt());
    info.setPrgId(header.value(QStringLiteral("prg_id")).toInt());
    info.setVersion(header.value(QStringLiteral("version")).toString());
    info.setName(header.value(QStringLiteral("name")).toString());
    info.setNumParams(header.value(QStringLiteral("num_param")).toInt());

    // params is an array of arrays: [["name", min, max, "type"], …]
    const QJsonArray paramsArr = header.value(QStringLiteral("params")).toArray();
    for (const QJsonValue &v : paramsArr) {
        const QJsonArray pa = v.toArray();
        if (pa.size() < 4) continue;

        UnitParam p;
        p.setName(pa.at(0).toString());
        p.setMin(pa.at(1).toInt());
        p.setMax(pa.at(2).toInt());
        p.setType(pa.at(3).toString());
        info.appendParam(p);
    }

    info.setValid(true);
    return info;
}

} // namespace qlogue