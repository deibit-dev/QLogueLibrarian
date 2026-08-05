#ifndef UNITINFO_H
#define UNITINFO_H

#include <QString>
#include <QVector>

namespace qlogue {

/// One parameter descriptor from manifest.json:
///   ["name", min, max, "type"]
struct UnitParam {
    QString  name;
    int      min  = 0;
    int      max  = 0;
    QString  type;              // e.g. "%", ""
};

/// Metadata extracted from the manifest.json inside a .xxxunit ZIP archive.
struct UnitInfo {
    // ── file ──────────────────────────────────────────────────────
    QString  filePath;
    QString  fileName;

    // ── manifest.json fields ──────────────────────────────────────
    QString  platform;          // "minilogue-xd", "prologue", …
    QString  module;            // "osc", "modfx", "delfx", "revfx", …
    QString  api;               // "1.1-0"
    int      devId    = 0;
    int      prgId    = 0;
    QString  version;           // "0.1-0"
    QString  name;
    int      numParams = 0;
    QVector<UnitParam> params;

    // ── status ────────────────────────────────────────────────────
    bool     isValid  = false;
};

} // namespace qlogue

#endif // UNITINFO_H
