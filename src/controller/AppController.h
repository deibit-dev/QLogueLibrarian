#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QString>

#include "logic/Logic.h"

namespace qlogue {

/// QML gateway (thin): the ONLY path through which the view mutates the
/// model/business layer. Validates parameters, delegates every operation to
/// Logic, and translates Logic's operation signals into user-facing
/// status/log text (written back to Logic). Holds no business logic and no
/// state of its own.
class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(Logic *logic, QObject *parent = nullptr);

    Q_INVOKABLE void probe();
    Q_INVOKABLE void setCliPath(const QString &path);
    Q_INVOKABLE void setUnitDir(const QString &dir);
    Q_INVOKABLE void loadUnit(const QString &unitPath, int slot,
                              int inRow, int outRow);

private:
    Logic *m_logic;
};

} // namespace qlogue

#endif // APPCONTROLLER_H
