#include "Dialogs.h"
#include "model/KorgEnums.h"

#include <QDir>
#include <QFileDialog>

namespace qlogue {

Dialogs::Dialogs(QObject *parent) : QObject(parent) {}

QString Dialogs::pickExecutable() {
    return QFileDialog::getOpenFileName(nullptr, tr("Select logue-cli"));
}

QString Dialogs::pickUnitFile() {
    return QFileDialog::getOpenFileName(nullptr, tr("Select unit file"), {}, unitFileFilter());
}

QString Dialogs::pickDirectory(const QString &startDir) {
    const QString start = startDir.isEmpty() ? QDir::homePath() : startDir;
    return QFileDialog::getExistingDirectory(nullptr, tr("Select Unit Directory"), start);
}

} // namespace qlogue
