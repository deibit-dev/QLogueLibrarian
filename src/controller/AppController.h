#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QStringList>
#include "model/PluginListModel.h"
#include "controller/LogueCLIWrapper.h"
#include "model/UnitInfo.h"

namespace qlogue {

class AppController : public QObject {
    Q_OBJECT

    // ── persistent paths ────────────────────────────────────────────
    Q_PROPERTY(QString cliPath    READ cliPath    WRITE setCliPath    NOTIFY cliPathChanged)
    Q_PROPERTY(QString pluginDir  READ pluginDir  WRITE setPluginDir  NOTIFY pluginDirChanged)

    // ── load unit ───────────────────────────────────────────────────
    Q_PROPERTY(QString unitPath   READ unitPath   WRITE setUnitPath   NOTIFY unitPathChanged)
    Q_PROPERTY(int     slot       READ slot       WRITE setSlot       NOTIFY slotChanged)

    // ── MIDI ports ──────────────────────────────────────────────────
    Q_PROPERTY(QStringList inPorts  READ inPorts  NOTIFY inPortsChanged)
    Q_PROPERTY(QStringList outPorts READ outPorts NOTIFY outPortsChanged)
    Q_PROPERTY(int inPortIndex  READ inPortIndex  WRITE setInPortIndex  NOTIFY inPortIndexChanged)
    Q_PROPERTY(int outPortIndex READ outPortIndex WRITE setOutPortIndex NOTIFY outPortIndexChanged)
    Q_PROPERTY(bool canLoad     READ canLoad      NOTIFY canLoadChanged)

    // ── plugin library ──────────────────────────────────────────────
    Q_PROPERTY(qlogue::PluginListModel* pluginListModel READ pluginListModel CONSTANT)

    // ── selected unit metadata ──────────────────────────────────────
    Q_PROPERTY(QString metaName      READ metaName      NOTIFY selectedUnitChanged)
    Q_PROPERTY(QString metaPlatform  READ metaPlatform  NOTIFY selectedUnitChanged)
    Q_PROPERTY(QString metaModule    READ metaModule    NOTIFY selectedUnitChanged)
    Q_PROPERTY(QString metaVersion   READ metaVersion   NOTIFY selectedUnitChanged)
    Q_PROPERTY(QString metaApi       READ metaApi       NOTIFY selectedUnitChanged)
    Q_PROPERTY(int     metaDevId     READ metaDevId     NOTIFY selectedUnitChanged)
    Q_PROPERTY(int     metaPrgId     READ metaPrgId     NOTIFY selectedUnitChanged)
    Q_PROPERTY(int     metaNumParams READ metaNumParams NOTIFY selectedUnitChanged)
    Q_PROPERTY(QVariantList metaParams READ metaParams  NOTIFY selectedUnitChanged)

    // ── status / log ────────────────────────────────────────────────
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString logText    READ logText    NOTIFY logTextChanged)

public:
    explicit AppController(QObject *parent = nullptr);

    // ── property getters ────────────────────────────────────────────
    QString cliPath()   const { return m_cliPath;   }
    QString pluginDir() const { return m_pluginDir; }
    QString unitPath()  const { return m_unitPath;  }
    int     slot()      const { return m_slot;      }

    QStringList inPorts()  const { return m_inPorts;  }
    QStringList outPorts() const { return m_outPorts; }
    int  inPortIndex()  const { return m_inPortIndex;  }
    int  outPortIndex() const { return m_outPortIndex; }
    bool canLoad()      const;

    PluginListModel *pluginListModel() { return &m_pluginModel; }

    QString metaName()      const;
    QString metaPlatform()  const;
    QString metaModule()    const;
    QString metaVersion()   const;
    QString metaApi()       const;
    int     metaDevId()     const;
    int     metaPrgId()     const;
    int     metaNumParams() const;
    QVariantList metaParams() const;

    QString statusText() const { return m_statusText; }
    QString logText()    const { return m_logText;    }

    // ── property setters ────────────────────────────────────────────
    void setCliPath(const QString &v);
    void setPluginDir(const QString &v);
    void setUnitPath(const QString &v);
    void setSlot(int v);
    void setInPortIndex(int v);
    void setOutPortIndex(int v);

    // ── invokable from QML ──────────────────────────────────────────
    Q_INVOKABLE void probe();
    Q_INVOKABLE void loadUnit();
    Q_INVOKABLE void selectPlugin(int row);
    Q_INVOKABLE void browseCliPath();
    Q_INVOKABLE void browseUnitFile();
    Q_INVOKABLE void browsePluginDir();

signals:
    void cliPathChanged();
    void pluginDirChanged();
    void unitPathChanged();
    void slotChanged();
    void inPortsChanged();
    void outPortsChanged();
    void inPortIndexChanged();
    void outPortIndexChanged();
    void canLoadChanged();
    void selectedUnitChanged();
    void statusTextChanged();
    void logTextChanged();

private:
    void appendLog(const QString &text);
    void setStatusText(const QString &text);
    void scanPluginDir(const QString &dirPath);
    int  autoSelectPort(const QStringList &list) const;

    LogueCLIWrapper m_cli;
    PluginListModel m_pluginModel;
    QStringList m_inPorts;
    QStringList m_outPorts;

    QString m_cliPath;
    QString m_pluginDir;
    QString m_unitPath;
    int     m_slot         = 0;
    int     m_inPortIndex  = -1;
    int     m_outPortIndex = -1;

    QVector<MidiPort> m_rawPorts;   // raw probe results for port index mapping

    UnitInfo m_selectedUnit;        // currently selected plugin metadata

    QString m_statusText;
    QString m_logText;
};

} // namespace qlogue

#endif // APPCONTROLLER_H
