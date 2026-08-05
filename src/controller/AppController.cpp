#include "AppController.h"
#include "UnitHeaderParser.h"
#include "model/KorgEnums.h"

#include <QDir>
#include <QFileDialog>
#include <QGuiApplication>
#include <QSettings>

namespace qlogue {

// ── file extensions that logue-cli recognises ───────────────────────────────
static const QStringList kUnitFilters = {
    QStringLiteral("*.prlgunit"),
    QStringLiteral("*.mnlgxdunit"),
    QStringLiteral("*.ntkdigunit"),
    QStringLiteral("*.drmlgunit"),
    QStringLiteral("*.nts1mkiiunit"),
    QStringLiteral("*.nts3unit"),
    QStringLiteral("*.mkg2unit"),
    QStringLiteral("*.unit"),
};

// ─── Constructor ────────────────────────────────────────────────────────────

AppController::AppController(QObject *parent)
    : QObject(parent)
{
    // restore persisted settings
    QSettings s;
    m_cliPath   = s.value(QStringLiteral("tools/logueCli"),
                          QStringLiteral("logue-cli")).toString();
    m_pluginDir = s.value(QStringLiteral("library/lastDir")).toString();

    // wire up LogueCLIWrapper signals
    connect(&m_cli, &LogueCLIWrapper::probeFinished, this, [this](QVector<MidiPort> ports){
        m_rawPorts = ports;

        QStringList inList, outList;
        for (const auto &p : ports) {
            auto label = QStringLiteral("%1: %2").arg(p.index).arg(p.name);
            if (p.direction == MidiPort::In)
                inList << label;
            else
                outList << label;
        }
        m_inPorts = inList;
        m_outPorts = outList;
        emit inPortsChanged();
        emit outPortsChanged();

        // auto-select ports containing "SOUND"
        setInPortIndex(autoSelectPort(inList));
        setOutPortIndex(autoSelectPort(outList));
        emit canLoadChanged();

        setStatusText(tr("Found %1 port(s)").arg(ports.size()));
        appendLog(tr("── Probe: %1 port(s) detected ──").arg(ports.size()));
    });

    connect(&m_cli, &LogueCLIWrapper::loadFinished, this, [this](LoadResult r){
        appendLog(r.rawOutput);
        if (r.success)
            setStatusText(tr("✔ Loaded on %1 – %2").arg(r.platform, r.module));
        else
            setStatusText(tr("✘ Upload failed"));
        emit canLoadChanged();
    });

    connect(&m_cli, &LogueCLIWrapper::errorOccurred, this, [this](QString msg){
        appendLog(QStringLiteral("ERROR: ") + msg);
        setStatusText(tr("Error"));
        emit canLoadChanged();
    });

    // scan persisted plugin dir on startup
    if (!m_pluginDir.isEmpty() && QDir(m_pluginDir).exists())
        scanPluginDir(m_pluginDir);
}

// ─── Property setters ───────────────────────────────────────────────────────

void AppController::setCliPath(const QString &v) {
    if (m_cliPath == v) return;
    m_cliPath = v;
    QSettings().setValue(QStringLiteral("tools/logueCli"), v);
    emit cliPathChanged();
}

void AppController::setPluginDir(const QString &v) {
    if (m_pluginDir == v) return;
    m_pluginDir = v;
    QSettings().setValue(QStringLiteral("library/lastDir"), v);
    emit pluginDirChanged();
    if (!v.isEmpty() && QDir(v).exists())
        scanPluginDir(v);
}

void AppController::setUnitPath(const QString &v) {
    if (m_unitPath == v) return;
    m_unitPath = v;
    emit unitPathChanged();
}

void AppController::setSlot(int v) {
    if (m_slot == v) return;
    m_slot = v;
    emit slotChanged();
}

void AppController::setInPortIndex(int v) {
    if (m_inPortIndex == v) return;
    m_inPortIndex = v;
    emit inPortIndexChanged();
    emit canLoadChanged();
}

void AppController::setOutPortIndex(int v) {
    if (m_outPortIndex == v) return;
    m_outPortIndex = v;
    emit outPortIndexChanged();
    emit canLoadChanged();
}

// ─── Metadata getters ───────────────────────────────────────────────────────

static const QString kDash = QStringLiteral("—");

QString AppController::metaName()      const { return m_selectedUnit.isValid ? m_selectedUnit.name.trimmed()   : kDash; }
QString AppController::metaPlatform()  const { return m_selectedUnit.isValid ? m_selectedUnit.platform         : kDash; }
QString AppController::metaModule()    const { return m_selectedUnit.isValid ? m_selectedUnit.module           : kDash; }
QString AppController::metaVersion()   const { return m_selectedUnit.isValid ? m_selectedUnit.version          : kDash; }
QString AppController::metaApi()       const { return m_selectedUnit.isValid ? m_selectedUnit.api              : kDash; }
int     AppController::metaDevId()     const { return m_selectedUnit.devId;     }
int     AppController::metaPrgId()     const { return m_selectedUnit.prgId;     }
int     AppController::metaNumParams() const { return m_selectedUnit.numParams; }

QVariantList AppController::metaParams() const {
    QVariantList list;
    for (const auto &p : m_selectedUnit.params) {
        list.append(QVariantMap{
            { QStringLiteral("name"), p.name },
            { QStringLiteral("min"),  p.min  },
            { QStringLiteral("max"),  p.max  },
            { QStringLiteral("type"), p.type },
        });
    }
    return list;
}

bool AppController::canLoad() const {
    return m_inPortIndex >= 0 && m_outPortIndex >= 0;
}

// ─── Invokable actions ──────────────────────────────────────────────────────

void AppController::probe() {
    m_cli.setCliPath(m_cliPath);
    m_inPorts.clear();
    m_outPorts.clear();
    emit inPortsChanged();
    emit outPortsChanged();
    setInPortIndex(-1);
    setOutPortIndex(-1);
    emit canLoadChanged();
    setStatusText(tr("Probing…"));
    m_cli.probe();
}

void AppController::loadUnit() {
    if (m_unitPath.isEmpty() || !canLoad()) return;

    // map combo index → raw MIDI port index
    int inIdx  = -1, outIdx = -1;
    int inCount = 0, outCount = 0;
    for (const auto &p : m_rawPorts) {
        if (p.direction == MidiPort::In) {
            if (inCount == m_inPortIndex) inIdx = p.index;
            ++inCount;
        } else {
            if (outCount == m_outPortIndex) outIdx = p.index;
            ++outCount;
        }
    }
    if (inIdx < 0 || outIdx < 0) return;

    int slotArg = (m_slot > 0) ? m_slot : -1;

    m_cli.setCliPath(m_cliPath);
    setStatusText(tr("Uploading…"));
    m_cli.loadUnit(m_unitPath, inIdx, outIdx, slotArg);
}

void AppController::selectPlugin(int row) {
    if (row < 0 || row >= m_pluginModel.count()) {
        m_selectedUnit = UnitInfo{};
        emit selectedUnitChanged();
        return;
    }
    m_selectedUnit = m_pluginModel.unitAt(row);
    setUnitPath(m_selectedUnit.filePath);
    emit selectedUnitChanged();
}

void AppController::browseCliPath() {
    auto p = QFileDialog::getOpenFileName(nullptr, tr("Select logue-cli"));
    if (!p.isEmpty())
        setCliPath(p);
}

void AppController::browseUnitFile() {
    auto p = QFileDialog::getOpenFileName(nullptr, tr("Select unit file"), {}, unitFileFilter());
    if (!p.isEmpty())
        setUnitPath(p);
}

void AppController::browsePluginDir() {
    auto dir = QFileDialog::getExistingDirectory(
        nullptr, tr("Select Plugin Directory"),
        m_pluginDir.isEmpty() ? QDir::homePath() : m_pluginDir);
    if (!dir.isEmpty())
        setPluginDir(dir);
}

// ─── Internal helpers ───────────────────────────────────────────────────────

void AppController::scanPluginDir(const QString &dirPath)
{
    QDir dir(dirPath);
    const QFileInfoList entries = dir.entryInfoList(kUnitFilters,
                                                     QDir::Files | QDir::Readable,
                                                     QDir::Name);
    QVector<UnitInfo> units;
    units.reserve(entries.size());
    for (const QFileInfo &fi : entries)
        units.append(UnitHeaderParser::parse(fi.absoluteFilePath()));

    m_pluginModel.setUnits(units);
    m_selectedUnit = UnitInfo{};
    emit selectedUnitChanged();

    setStatusText(tr("Library: %1 plugin(s) found").arg(units.size()));
    appendLog(tr("── Plugin dir scanned: %1 file(s) ──").arg(units.size()));
}

int AppController::autoSelectPort(const QStringList &list) const
{
    for (int i = 0; i < list.size(); ++i) {
        if (list.at(i).contains(QLatin1String("SOUND"), Qt::CaseInsensitive))
            return i;
    }
    return list.isEmpty() ? -1 : 0;
}

void AppController::appendLog(const QString &text) {
    if (!m_logText.isEmpty())
        m_logText += QLatin1Char('\n');
    m_logText += text;
    emit logTextChanged();
}

void AppController::setStatusText(const QString &text) {
    if (m_statusText == text) return;
    m_statusText = text;
    emit statusTextChanged();
}

} // namespace qlogue
