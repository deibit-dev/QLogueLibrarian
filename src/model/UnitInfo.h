#ifndef UNITINFO_H
#define UNITINFO_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVector>

namespace qlogue {

/// One parameter descriptor from manifest.json: ["name", min, max, "type"].
///
/// Registered as a QML value type (Q_GADGET, see main.cpp) so the view can
/// read a parameter's fields directly from the model. Logic (the model) therefore
/// does not need to flatten each field into a "meta*"-style property.
class UnitParam {
    Q_GADGET
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(int min READ min CONSTANT)
    Q_PROPERTY(int max READ max CONSTANT)
    Q_PROPERTY(QString type READ type CONSTANT)

public:
    // Read-only accessors, reused as the QML bindings above. The value type
    // is immutable from the outside: only UnitHeaderParser populates it.
    QString name() const { return m_name; }
    int min() const { return m_min; }
    int max() const { return m_max; }
    QString type() const { return m_type; }

    // Setters used only by UnitHeaderParser while populating from JSON.
    void setName(const QString &v) { m_name = v; }
    void setMin(int v) { m_min = v; }
    void setMax(int v) { m_max = v; }
    void setType(const QString &v) { m_type = v; }

private:
    QString m_name;
    int m_min = 0;
    int m_max = 0;
    QString m_type;            // e.g. "%", ""
};

/// Metadata extracted from the manifest.json inside a .xxxunit ZIP archive.
///
/// This is the single source of truth for a unit. Logic stores a vector
/// of these and exposes them to QML as a typed value type (Q_GADGET); the
/// previous design flattened a copy of one unit into nine loose "meta*"
/// Q_PROPERTYs, which duplicated data and leaked presentation logic into
/// the model. Grouping the fields here keeps the boundary encapsulated.
class UnitInfo {
    Q_GADGET
    Q_PROPERTY(QString filePath READ filePath CONSTANT)
    Q_PROPERTY(QString fileName READ fileName CONSTANT)
    Q_PROPERTY(QString platform READ platform CONSTANT)
    Q_PROPERTY(QString module READ module CONSTANT)
    Q_PROPERTY(QString api READ api CONSTANT)
    Q_PROPERTY(int devId READ devId CONSTANT)
    Q_PROPERTY(int prgId READ prgId CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(int numParams READ numParams CONSTANT)
    Q_PROPERTY(QVariantList params READ params CONSTANT)
    Q_PROPERTY(bool isValid READ isValid CONSTANT)

public:
    // Read-only accessors, reused as the QML bindings above.
    QString filePath() const { return m_filePath; }
    QString fileName() const { return m_fileName; }
    QString platform() const { return m_platform; }
    QString module() const { return m_module; }
    QString api() const { return m_api; }
    int devId() const { return m_devId; }
    int prgId() const { return m_prgId; }
    QString version() const { return m_version; }
    QString name() const { return m_name; }
    int numParams() const { return m_numParams; }
    bool isValid() const { return m_isValid; }

    // Params projected as a QVariantList of UnitParam value types so QML
    // can iterate them (UnitParam's own fields stay encapsulated).
    QVariantList params() const;

    // Setters used only by UnitHeaderParser while populating from JSON.
    void setFilePath(const QString &v) { m_filePath = v; }
    void setFileName(const QString &v) { m_fileName = v; }
    void setPlatform(const QString &v) { m_platform = v; }
    void setModule(const QString &v) { m_module = v; }
    void setApi(const QString &v) { m_api = v; }
    void setDevId(int v) { m_devId = v; }
    void setPrgId(int v) { m_prgId = v; }
    void setVersion(const QString &v) { m_version = v; }
    void setName(const QString &v) { m_name = v; }
    void setNumParams(int v) { m_numParams = v; }
    void appendParam(const UnitParam &p) { m_params.append(p); }
    void setValid(bool v) { m_isValid = v; }

private:
    QString m_filePath;
    QString m_fileName;

    QString m_platform;        // "minilogue-xd", "prologue", …
    QString m_module;          // "osc", "modfx", "delfx", "revfx", …
    QString m_api;             // "1.1-0"
    int m_devId = 0;
    int m_prgId = 0;
    QString m_version;         // "0.1-0"
    QString m_name;
    int m_numParams = 0;
    QVector<UnitParam> m_params;
    bool m_isValid = false;
};

} // namespace qlogue

// Required so QVariant::fromValue() can box these value types for QML.
Q_DECLARE_METATYPE(qlogue::UnitParam)
Q_DECLARE_METATYPE(qlogue::UnitInfo)

#endif // UNITINFO_H
