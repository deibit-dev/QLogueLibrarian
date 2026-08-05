#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include "controller/LogueCLIWrapper.h"
#include "model/UnitInfo.h"

class QComboBox;
class QPlainTextEdit;
class QLabel;
class QPushButton;
class QLineEdit;
class QSpinBox;
class QListWidget;
class QTableWidget;
class QFormLayout;
class QStackedWidget;

namespace qlogue {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onProbe();
    void onProbeResult(QVector<qlogue::MidiPort> ports);
    void onBrowseUnit();
    void onLoad();
    void onLoadResult(qlogue::LoadResult result);
    void onError(QString msg);

    // plugin library
    void onBrowsePluginDir();
    void onPluginSelected(int row);

private:
    LogueCLIWrapper *m_cli;

    // toolbar / path
    QLineEdit *m_cliPathEdit;

    // probe section
    QComboBox *m_inPortCombo;
    QComboBox *m_outPortCombo;

    // load section
    QLineEdit   *m_unitPathEdit;
    QSpinBox    *m_slotSpin;      // -1 = auto (value 0), 1..N = explicit slot
    QPushButton *m_loadBtn;

    // plugin library section
    QLineEdit   *m_pluginDirEdit;
    QListWidget *m_pluginList;

    // metadata panel (right side of library section)
    QLabel      *m_metaName;
    QLabel      *m_metaPlatform;
    QLabel      *m_metaModule;
    QLabel      *m_metaVersion;
    QLabel      *m_metaApi;
    QLabel      *m_metaDevId;
    QLabel      *m_metaPrgId;
    QLabel      *m_metaNumParams;
    QTableWidget *m_paramsTable;

    // log
    QPlainTextEdit *m_log;
    QLabel         *m_statusLabel;

    QVector<MidiPort> m_ports;
    QVector<UnitInfo> m_units;   // parsed headers for current plugin dir

    void appendLog(const QString &text);
    void scanPluginDir(const QString &dirPath);
    void showUnitMeta(const UnitInfo &info);
    void clearUnitMeta();
    QWidget *buildMetaPanel();
};

} // namespace qlogue

#endif // MAINWINDOW_H
