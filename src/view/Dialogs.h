#ifndef DIALOGS_H
#define DIALOGS_H

#include <QObject>
#include <QString>

namespace qlogue {

/// View-layer helper: opens native file/folder dialogs and returns the
/// chosen path (empty if cancelled). Holds no state.
class Dialogs : public QObject {
    Q_OBJECT
public:
    explicit Dialogs(QObject *parent = nullptr);

    Q_INVOKABLE QString pickExecutable();
    Q_INVOKABLE QString pickUnitFile();
    Q_INVOKABLE QString pickDirectory(const QString &startDir = {});
};

} // namespace qlogue

#endif // DIALOGS_H
