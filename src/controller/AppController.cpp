#include "AppController.h"

#include <QDir>

namespace qlogue {

AppController::AppController(Logic *logic, QObject *parent)
    : QObject(parent), m_logic(logic)
{
    // Translate Logic's business outcomes into presentation (status/log).
    // Logic stays free of user-facing strings; it only reports typed results.
    connect(m_logic, &Logic::probeFinished, this, [this](int portCount) {
        m_logic->setStatus(tr("Found %1 port(s)").arg(portCount));
        m_logic->appendLog(tr("── Probe: %1 port(s) detected ──").arg(portCount));
    });

    connect(m_logic, &Logic::scanFinished, this, [this](int unitCount) {
        m_logic->setStatus(tr("Unit library: %1 unit(s) found").arg(unitCount));
        m_logic->appendLog(tr("── Unit dir scanned: %1 file(s) ──").arg(unitCount));
    });

    connect(m_logic, &Logic::loadFinished, this, [this](LoadResult r) {
        m_logic->appendLog(r.rawOutput);
        if (r.success)
            m_logic->setStatus(tr("✔ Loaded on %1 – %2").arg(r.platform, r.module));
        else
            m_logic->setStatus(tr("✘ Upload failed"));
    });

    connect(m_logic, &Logic::errorOccurred, this, [this](QString msg) {
        m_logic->appendLog(QStringLiteral("ERROR: ") + msg);
        m_logic->setStatus(tr("Error"));
    });

    // Startup behaviour (as before): probe the ports and rescan the persisted
    // unit directory if there is one.
    probe();
    if (!m_logic->unitDir().isEmpty() && QDir(m_logic->unitDir()).exists())
        m_logic->scanUnits(m_logic->unitDir());
}

// ─── Invokable actions ──────────────────────────────────────────────────────

void AppController::probe() {
    m_logic->setStatus(tr("Probing…"));
    m_logic->probe();
}

void AppController::setCliPath(const QString &path) {
    m_logic->setCliPath(path);
}

void AppController::setUnitDir(const QString &dir) {
    m_logic->setUnitDir(dir);
}

void AppController::loadUnit(const QString &unitPath, int slot,
                             int inRow, int outRow) {
    if (unitPath.isEmpty())
        return;

    if (m_logic->loadUnit(unitPath, slot, inRow, outRow))
        m_logic->setStatus(tr("Uploading…"));
}

} // namespace qlogue
