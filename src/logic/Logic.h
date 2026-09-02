#ifndef LOGIC_H
#define LOGIC_H

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVector>

#include "logic/LogueCLIWrapper.h"
#include "model/MidiPort.h"
#include "model/UnitInfo.h"

namespace qlogue {

/// Application state + business orchestrator (was AppState, merged).
///
/// This object is the single entry point for business operations AND the
/// observable store exposed to QML ("App"). It owns its data (persisted
/// settings, probed MIDI ports, the unit library) and the operations that
/// mutate it (probe / scan / load), so there is no ambiguity about who
/// writes the model: Logic mutates itself.
///
/// It deliberately does NOT store presentation state (which unit/port is
/// selected, the current load target, the slot) — that belongs to the view
/// (ViewState). Business operations are NOT Q_INVOKABLE: the view reaches
/// them only through AppController (single command path), which translates
/// the operation signals below into user-facing status/log text.
///
/// Every property is read-only for QML (no WRITE): the view can only bind.
class Logic : public QObject {
    Q_OBJECT

    // ── persistent settings (read-only from QML) ───────────────────────
    Q_PROPERTY(QString cliPath READ cliPath NOTIFY cliPathChanged)
    Q_PROPERTY(QString unitDir READ unitDir NOTIFY unitDirChanged)

    // ── MIDI ports (domain data from `probe -l`) ───────────────────────
    Q_PROPERTY(QStringList inPorts  READ inPorts  NOTIFY inPortsChanged)
    Q_PROPERTY(QStringList outPorts READ outPorts NOTIFY outPortsChanged)

    // ── unit library (single source of truth) ──────────────────────────
    Q_PROPERTY(QVariantList library READ library NOTIFY libraryChanged)

    // ── status / log (written by AppController via appendLog/setStatus) ─
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString logText    READ logText    NOTIFY logTextChanged)

public:
    explicit Logic(QObject *parent = nullptr);

    // property getters
    QString cliPath()   const { return m_cliPath; }
    QString unitDir()   const { return m_unitDir; }

    QStringList inPorts()  const;
    QStringList outPorts() const;

    QVariantList library() const;

    QString statusText() const { return m_statusText; }
    QString logText()    const { return m_logText; }

    // ── Business operations (orchestration; called by AppController) ────
    // Launch `probe -l` and refresh the port lists on completion.
    void probe();

    /// Upload @p unitPath. Ports are the view's chosen label rows, resolved
    /// here to raw MIDI indices. Returns false if the request is invalid
    /// (nothing is launched); true once the async process started.
    bool loadUnit(const QString &unitPath, int slot, int inRow, int outRow);

    /// Rescan @p dir for .xxxunit files and refresh the library.
    void scanUnits(const QString &dir);

    /// Update the persisted logue-cli path (settings write is internal).
    void setCliPath(const QString &v);

    /// Update the persisted unit directory and rescan it if it exists.
    void setUnitDir(const QString &v);

    // ── controller-facing feedback (presentation translation) ───────────
    void appendLog(const QString &text);
    void setStatus(const QString &text);

    // ── resolution helpers for AppController ────────────────────────────
    int resolveInPort(int row) const;   // label row → raw MIDI index
    int resolveOutPort(int row) const;
    int unitCount() const { return m_units.size(); }

signals:
    // property notifications
    void cliPathChanged();
    void unitDirChanged();
    void inPortsChanged();
    void outPortsChanged();
    void libraryChanged();
    void statusTextChanged();
    void logTextChanged();

    // operation outcomes — Logic does NOT compose user text; AppController
    // translates these into statusText/logText.
    void probeFinished(int portCount);
    void scanFinished(int unitCount);
    void loadFinished(qlogue::LoadResult result);
    void errorOccurred(QString message);

private:
    // controller-facing mutations of domain state (wired to LogueCLIWrapper)
    void setPorts(const QVector<MidiPort> &ports);
    void clearPorts();
    void setUnits(const QVector<UnitInfo> &units);

    static QStringList labelsFor(const QVector<MidiPort> &ports);

    LogueCLIWrapper m_cli;      // owned; async QProcess runner

    QVector<MidiPort> m_inPorts;
    QVector<MidiPort> m_outPorts;

    QVector<UnitInfo> m_units;   // canonical library; `library()` projects it

    QString m_cliPath;
    QString m_unitDir;

    QString m_statusText;
    QString m_logText;
};

} // namespace qlogue

#endif // LOGIC_H
