#include "MainWindow.h"
#include "model/KorgEnums.h"
#include "controller/UnitHeaderParser.h"

#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QStringList>
#include <QTableWidget>
#include <QVBoxLayout>

namespace qlogue {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_cli(new LogueCLIWrapper(this))
{
    setWindowTitle(QStringLiteral("QLogueLibrarian"));
    resize(720, 480);

    auto *central = new QWidget;
    auto *root    = new QVBoxLayout(central);

    // ── logue-cli path ──────────────────────────────────────────────────
    {
        auto *row = new QHBoxLayout;
        row->addWidget(new QLabel(tr("logue-cli:")));
        m_cliPathEdit = new QLineEdit;
        m_cliPathEdit->setText(
            QSettings().value(QStringLiteral("tools/logueCli"),
                              QStringLiteral("logue-cli")).toString());
        connect(m_cliPathEdit, &QLineEdit::textChanged, this, [](const QString &text){
            QSettings().setValue(QStringLiteral("tools/logueCli"), text);
        });
        row->addWidget(m_cliPathEdit, 1);
        auto *browseBtn = new QPushButton(tr("..."));
        connect(browseBtn, &QPushButton::clicked, this, [this]{
            auto p = QFileDialog::getOpenFileName(this, tr("Select logue-cli"));
            if (!p.isEmpty()) m_cliPathEdit->setText(p);
        });
        row->addWidget(browseBtn);
        root->addLayout(row);
    }

    // ── Probe section ───────────────────────────────────────────────────
    {
        auto *grp = new QGroupBox(tr("MIDI Ports"));
        auto *lay = new QVBoxLayout(grp);

        auto *probeBtn = new QPushButton(tr("Probe"));
        connect(probeBtn, &QPushButton::clicked, this, &MainWindow::onProbe);
        lay->addWidget(probeBtn);

        auto *row = new QHBoxLayout;
        row->addWidget(new QLabel(tr("In:")));
        m_inPortCombo = new QComboBox;
        row->addWidget(m_inPortCombo, 1);
        row->addWidget(new QLabel(tr("Out:")));
        m_outPortCombo = new QComboBox;
        row->addWidget(m_outPortCombo, 1);
        lay->addLayout(row);

        root->addWidget(grp);
    }

    // ── Load section ────────────────────────────────────────────────────
    {
        auto *grp = new QGroupBox(tr("Load Unit"));
        auto *lay = new QHBoxLayout(grp);

        m_unitPathEdit = new QLineEdit;
        m_unitPathEdit->setPlaceholderText(tr("Select a .xxxunit file…"));
        lay->addWidget(m_unitPathEdit, 1);

        auto *browseBtn = new QPushButton(tr("Browse…"));
        connect(browseBtn, &QPushButton::clicked, this, &MainWindow::onBrowseUnit);
        lay->addWidget(browseBtn);

        lay->addWidget(new QLabel(tr("Slot:")));
        m_slotSpin = new QSpinBox;
        m_slotSpin->setMinimum(0);
        m_slotSpin->setMaximum(16);
        m_slotSpin->setValue(0);
        m_slotSpin->setSpecialValueText(tr("Auto"));  // 0 → auto (no -s flag)
        m_slotSpin->setToolTip(tr("0 = first available slot (auto)"));
        lay->addWidget(m_slotSpin);

        m_loadBtn = new QPushButton(tr("Upload"));
        m_loadBtn->setEnabled(false);
        connect(m_loadBtn, &QPushButton::clicked, this, &MainWindow::onLoad);
        lay->addWidget(m_loadBtn);

        root->addWidget(grp);
    }

    // ── Log ─────────────────────────────────────────────────────────────
    m_log = new QPlainTextEdit;
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(500);

    // ── Plugin Library section ───────────────────────────────────────────
    auto *libGrp = new QGroupBox(tr("Plugin Library"));
    {
        auto *lay = new QVBoxLayout(libGrp);

        // directory picker row
        auto *dirRow = new QHBoxLayout;
        dirRow->addWidget(new QLabel(tr("Directory:")));
        m_pluginDirEdit = new QLineEdit;
        m_pluginDirEdit->setPlaceholderText(tr("Select plugin directory…"));
        m_pluginDirEdit->setText(
            QSettings().value(QStringLiteral("library/lastDir")).toString());
        dirRow->addWidget(m_pluginDirEdit, 1);
        auto *dirBtn = new QPushButton(tr("Browse…"));
        connect(dirBtn, &QPushButton::clicked, this, &MainWindow::onBrowsePluginDir);
        dirRow->addWidget(dirBtn);
        lay->addLayout(dirRow);

        // splitter: plugin list (left) + meta panel (right)
        auto *splitter = new QSplitter(Qt::Horizontal);

        m_pluginList = new QListWidget;
        m_pluginList->setMinimumWidth(180);
        connect(m_pluginList, &QListWidget::currentRowChanged,
                this, &MainWindow::onPluginSelected);
        splitter->addWidget(m_pluginList);
        splitter->addWidget(buildMetaPanel());
        splitter->setStretchFactor(0, 1);
        splitter->setStretchFactor(1, 2);

        lay->addWidget(splitter, 1);
    }

    // ── Assemble root layout ─────────────────────────────────────────────
    root->addWidget(libGrp, 2);
    root->addWidget(m_log, 1);

    // ── Status bar ──────────────────────────────────────────────────────
    m_statusLabel = new QLabel;
    statusBar()->addWidget(m_statusLabel, 1);

    setCentralWidget(central);

    // ── Signals ─────────────────────────────────────────────────────────
    connect(m_cli, &LogueCLIWrapper::probeFinished,  this, &MainWindow::onProbeResult);
    connect(m_cli, &LogueCLIWrapper::loadFinished,   this, &MainWindow::onLoadResult);
    connect(m_cli, &LogueCLIWrapper::errorOccurred,  this, &MainWindow::onError);

    // ── Restore last plugin directory ───────────────────────────────────
    const QString lastDir = m_pluginDirEdit->text();
    if (!lastDir.isEmpty() && QDir(lastDir).exists())
        scanPluginDir(lastDir);
}

// ─── Slots ──────────────────────────────────────────────────────────────────

void MainWindow::onProbe() {
    m_cli->setCliPath(m_cliPathEdit->text());
    m_inPortCombo->clear();
    m_outPortCombo->clear();
    m_loadBtn->setEnabled(false);
    m_statusLabel->setText(tr("Probing…"));
    m_cli->probe();
}

void MainWindow::onProbeResult(QVector<MidiPort> ports) {
    m_ports = ports;
    for (auto &p : ports) {
        auto label = QStringLiteral("%1: %2").arg(p.index).arg(p.name);
        if (p.direction == MidiPort::In)
            m_inPortCombo->addItem(label, p.index);
        else
            m_outPortCombo->addItem(label, p.index);
    }
    // Auto-select ports whose name contains "SOUND"
    for (int i = 0; i < m_inPortCombo->count(); ++i) {
        if (m_inPortCombo->itemText(i).contains(QLatin1String("SOUND"), Qt::CaseInsensitive)) {
            m_inPortCombo->setCurrentIndex(i);
            break;
        }
    }
    for (int i = 0; i < m_outPortCombo->count(); ++i) {
        if (m_outPortCombo->itemText(i).contains(QLatin1String("SOUND"), Qt::CaseInsensitive)) {
            m_outPortCombo->setCurrentIndex(i);
            break;
        }
    }
    m_loadBtn->setEnabled(m_inPortCombo->count() > 0
                          && m_outPortCombo->count() > 0);
    m_statusLabel->setText(tr("Found %1 port(s)").arg(ports.size()));
    appendLog(tr("── Probe: %1 port(s) detected ──").arg(ports.size()));
}

void MainWindow::onBrowseUnit() {
    auto path = QFileDialog::getOpenFileName(
        this, tr("Select unit file"), {}, unitFileFilter());
    if (!path.isEmpty())
        m_unitPathEdit->setText(path);
}

void MainWindow::onLoad() {
    auto unit = m_unitPathEdit->text();
    if (unit.isEmpty()) return;

    int in   = m_inPortCombo->currentData().toInt();
    int out  = m_outPortCombo->currentData().toInt();
    // spinbox value 0 means "auto" → pass -1 so LogueCLIWrapper omits -s
    int slot = (m_slotSpin->value() > 0) ? m_slotSpin->value() : -1;

    m_cli->setCliPath(m_cliPathEdit->text());
    m_statusLabel->setText(tr("Uploading…"));
    m_loadBtn->setEnabled(false);
    m_cli->loadUnit(unit, in, out, slot);
}

void MainWindow::onLoadResult(LoadResult r) {
    appendLog(r.rawOutput);
    if (r.success) {
        m_statusLabel->setText(
            tr("✔ Loaded on %1 – %2").arg(r.platform, r.module));
    } else {
        m_statusLabel->setText(tr("✘ Upload failed"));
    }
    m_loadBtn->setEnabled(true);
}

void MainWindow::onError(QString msg) {
    appendLog(QStringLiteral("ERROR: ") + msg);
    m_statusLabel->setText(tr("Error"));
    m_loadBtn->setEnabled(true);
}

void MainWindow::appendLog(const QString &text) {
    m_log->appendPlainText(text);
}

// ─── Plugin Library ──────────────────────────────────────────────────────────

QWidget *MainWindow::buildMetaPanel()
{
    auto *panel = new QWidget;
    auto *vlay  = new QVBoxLayout(panel);
    vlay->setContentsMargins(4, 0, 0, 0);

    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight);

    auto makeLabel = [](){ auto *l = new QLabel(QStringLiteral("—")); l->setWordWrap(true); return l; };

    m_metaName      = makeLabel(); form->addRow(tr("Name:"),     m_metaName);
    m_metaPlatform  = makeLabel(); form->addRow(tr("Platform:"), m_metaPlatform);
    m_metaModule    = makeLabel(); form->addRow(tr("Module:"),   m_metaModule);
    m_metaVersion   = makeLabel(); form->addRow(tr("Version:"),  m_metaVersion);
    m_metaApi       = makeLabel(); form->addRow(tr("API:"),      m_metaApi);
    m_metaDevId     = makeLabel(); form->addRow(tr("Dev ID:"),   m_metaDevId);
    m_metaPrgId     = makeLabel(); form->addRow(tr("Prg ID:"),   m_metaPrgId);
    m_metaNumParams = makeLabel(); form->addRow(tr("Params:"),   m_metaNumParams);
    vlay->addLayout(form);

    // parameter table
    m_paramsTable = new QTableWidget(0, 4);
    m_paramsTable->setHorizontalHeaderLabels(
        {tr("Name"), tr("Min"), tr("Max"), tr("Type")});
    m_paramsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_paramsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_paramsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_paramsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_paramsTable->setAlternatingRowColors(true);
    vlay->addWidget(m_paramsTable, 1);

    return panel;
}

void MainWindow::onBrowsePluginDir()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("Select Plugin Directory"),
        QSettings().value(QStringLiteral("library/lastDir"), QDir::homePath()).toString());
    if (dir.isEmpty()) return;
    QSettings().setValue(QStringLiteral("library/lastDir"), dir);
    m_pluginDirEdit->setText(dir);
    scanPluginDir(dir);
}

void MainWindow::scanPluginDir(const QString &dirPath)
{
    m_units.clear();
    m_pluginList->clear();
    clearUnitMeta();

    // file extensions that logue-cli recognises
    static const QStringList filters = {
        QStringLiteral("*.prlgunit"),
        QStringLiteral("*.mnlgxdunit"),
        QStringLiteral("*.ntkdigunit"),
        QStringLiteral("*.drmlgunit"),
        QStringLiteral("*.nts1mkiiunit"),
        QStringLiteral("*.nts3unit"),
        QStringLiteral("*.mkg2unit"),
        QStringLiteral("*.unit"),
    };

    QDir dir(dirPath);
    const QFileInfoList entries = dir.entryInfoList(filters,
                                                     QDir::Files | QDir::Readable,
                                                     QDir::Name);

    for (const QFileInfo &fi : entries) {
        UnitInfo info = UnitHeaderParser::parse(fi.absoluteFilePath());
        m_units.append(info);

        // display the unit name from header (fallback to filename)
        const QString label = info.isValid && !info.name.trimmed().isEmpty()
                              ? info.name.trimmed()
                              : fi.fileName();
        auto *item = new QListWidgetItem(label, m_pluginList);
        if (!info.isValid)
            item->setToolTip(tr("Could not parse header"));
    }

    m_statusLabel->setText(tr("Library: %1 plugin(s) found").arg(m_units.size()));
    appendLog(tr("── Plugin dir scanned: %1 file(s) ──").arg(m_units.size()));
}

void MainWindow::onPluginSelected(int row)
{
    if (row < 0 || row >= m_units.size()) {
        clearUnitMeta();
        return;
    }
    const UnitInfo &info = m_units.at(row);
    showUnitMeta(info);
    m_unitPathEdit->setText(info.filePath);
}

void MainWindow::showUnitMeta(const UnitInfo &info)
{
    if (!info.isValid) {
        clearUnitMeta();
        m_metaName->setText(tr("(invalid header)"));
        return;
    }

    m_metaName->setText(info.name.trimmed().isEmpty()
                        ? tr("(no name)") : info.name.trimmed());
    m_metaPlatform->setText(info.platform);
    m_metaModule->setText(info.module);
    m_metaVersion->setText(info.version);
    m_metaApi->setText(info.api);
    m_metaDevId->setText(QString::number(info.devId));
    m_metaPrgId->setText(QString::number(info.prgId));
    m_metaNumParams->setText(QString::number(info.numParams));

    // populate params table
    m_paramsTable->setRowCount(static_cast<int>(info.params.size()));
    for (int i = 0; i < info.params.size(); ++i) {
        const UnitParam &p = info.params.at(i);
        m_paramsTable->setItem(i, 0, new QTableWidgetItem(p.name));
        m_paramsTable->setItem(i, 1, new QTableWidgetItem(QString::number(p.min)));
        m_paramsTable->setItem(i, 2, new QTableWidgetItem(QString::number(p.max)));
        m_paramsTable->setItem(i, 3, new QTableWidgetItem(p.type));
    }
}

void MainWindow::clearUnitMeta()
{
    const QString dash = QStringLiteral("—");
    m_metaName->setText(dash);
    m_metaPlatform->setText(dash);
    m_metaModule->setText(dash);
    m_metaVersion->setText(dash);
    m_metaApi->setText(dash);
    m_metaDevId->setText(dash);
    m_metaPrgId->setText(dash);
    m_metaNumParams->setText(dash);
    m_paramsTable->setRowCount(0);
}


} // namespace qlogue
