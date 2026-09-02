#include "Logic.h"

#include "logic/UnitHeaderParser.h"
#include "model/KorgEnums.h"

#include <QDir>
#include <QSettings>
#include <QVariant>

namespace qlogue {

Logic::Logic(QObject *parent)
    : QObject(parent)
{
    QSettings s;
    m_cliPath = s.value(QStringLiteral("tools/logueCli"),
                        QStringLiteral("logue-cli")).toString();
    m_unitDir = s.value(QStringLiteral("library/lastDir")).toString();

    // Wire the async CLI runner to our own domain state and operation
    // signals. Logic mutates itself (setPorts/setUnits) and reports outcomes
    // through the operation signals; AppController translates those into
    // user-facing status/log text.
    connect(&m_cli, &LogueCLIWrapper::probeFinished, this,
            [this](QVector<MidiPort> ports) {
        setPorts(ports);
        emit probeFinished(ports.size());
    });
    connect(&m_cli, &LogueCLIWrapper::loadFinished,
            this, &Logic::loadFinished);
    connect(&m_cli, &LogueCLIWrapper::errorOccurred,
            this, &Logic::errorOccurred);
}

// ── Business operations ─────────────────────────────────────────────────────

void Logic::probe() {
    clearPorts();
    m_cli.probe(m_cliPath);
}

bool Logic::loadUnit(const QString &unitPath, int slot, int inRow, int outRow) {
    if (unitPath.isEmpty()) return false;

    const int inIdx  = resolveInPort(inRow);
    const int outIdx = resolveOutPort(outRow);
    if (inIdx < 0 || outIdx < 0) return false;

    const int slotArg = (slot > 0) ? slot : -1;   // 0 = auto

    m_cli.loadUnit(m_cliPath, unitPath, inIdx, outIdx, slotArg);
    return true;
}

void Logic::scanUnits(const QString &dir) {
    if (dir.isEmpty() || !QDir(dir).exists())
        return;

    QDir d(dir);
    const QFileInfoList entries = d.entryInfoList(unitExtensions(),
                                                   QDir::Files | QDir::Readable,
                                                   QDir::Name);
    QVector<UnitInfo> units;
    units.reserve(entries.size());
    for (const QFileInfo &fi : entries)
        units.append(UnitHeaderParser::parse(fi.absoluteFilePath()));

    setUnits(units);
    emit scanFinished(units.size());
}

void Logic::setCliPath(const QString &v) {
    if (m_cliPath == v) return;
    m_cliPath = v;
    QSettings().setValue(QStringLiteral("tools/logueCli"), v);
    emit cliPathChanged();
}

void Logic::setUnitDir(const QString &v) {
    if (m_unitDir == v) return;
    m_unitDir = v;
    QSettings().setValue(QStringLiteral("library/lastDir"), v);
    emit unitDirChanged();
    scanUnits(v);   // no-op (silently) when the directory does not exist
}

// ── MIDI ports ──────────────────────────────────────────────────────────────

void Logic::setPorts(const QVector<MidiPort> &ports) {
    m_inPorts.clear();
    m_outPorts.clear();
    for (const MidiPort &p : ports) {
        if (p.direction == MidiPort::In) m_inPorts.append(p);
        else                             m_outPorts.append(p);
    }
    emit inPortsChanged();
    emit outPortsChanged();
}

void Logic::clearPorts() {
    m_inPorts.clear();
    m_outPorts.clear();
    emit inPortsChanged();
    emit outPortsChanged();
}

QStringList Logic::labelsFor(const QVector<MidiPort> &ports) {
    QStringList labels;
    labels.reserve(ports.size());
    for (const MidiPort &p : ports)
        labels << QStringLiteral("%1: %2").arg(p.index).arg(p.name);
    return labels;
}

QStringList Logic::inPorts()  const { return labelsFor(m_inPorts); }
QStringList Logic::outPorts() const { return labelsFor(m_outPorts); }

int Logic::resolveInPort(int row) const {
    return (row >= 0 && row < m_inPorts.size()) ? m_inPorts.at(row).index : -1;
}

int Logic::resolveOutPort(int row) const {
    return (row >= 0 && row < m_outPorts.size()) ? m_outPorts.at(row).index : -1;
}

// ── unit library ────────────────────────────────────────────────────────────

// m_units is the single source of truth. `library()` projects it to a
// QVariantList of UnitInfo value types on demand (no second container).
void Logic::setUnits(const QVector<UnitInfo> &units) {
    m_units = units;
    emit libraryChanged();
}

QVariantList Logic::library() const {
    QVariantList list;
    list.reserve(m_units.size());
    for (const UnitInfo &u : m_units)
        list.append(QVariant::fromValue(u));
    return list;
}

// ── feedback ────────────────────────────────────────────────────────────────

void Logic::appendLog(const QString &text) {
    if (!m_logText.isEmpty())
        m_logText += QLatin1Char('\n');
    m_logText += text;
    emit logTextChanged();
}

void Logic::setStatus(const QString &text) {
    if (m_statusText == text) return;
    m_statusText = text;
    emit statusTextChanged();
}

} // namespace qlogue
