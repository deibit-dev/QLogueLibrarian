#ifndef VIEWSTATE_H
#define VIEWSTATE_H

#include <QObject>
#include <QString>

namespace qlogue {

/// View-layer presentation state.
///
/// In the MVC split the model owns domain data and the view owns selection /
/// "current item" / form values. This QObject centralises that presentation
/// state so the QML components (list, meta panel, load form, port pickers)
/// share one selection instead of each keeping its own, and so the model
/// (Logic) stays free of selection logic.
///
/// The controller reads nothing from here directly; the view passes these
/// values to the controller's invokables as arguments.
class ViewState : public QObject {
    Q_OBJECT

    Q_PROPERTY(int libraryIndex READ libraryIndex WRITE setLibraryIndex NOTIFY libraryIndexChanged)
    Q_PROPERTY(QString unitPath READ unitPath WRITE setUnitPath NOTIFY unitPathChanged)
    Q_PROPERTY(int inIndex READ inIndex WRITE setInIndex NOTIFY inIndexChanged)
    Q_PROPERTY(int outIndex READ outIndex WRITE setOutIndex NOTIFY outIndexChanged)
    Q_PROPERTY(int slot READ slot WRITE setSlot NOTIFY slotChanged)
    Q_PROPERTY(bool canLoad READ canLoad NOTIFY canLoadChanged)

public:
    explicit ViewState(QObject *parent = nullptr);

    int libraryIndex() const { return m_libraryIndex; }
    QString unitPath() const { return m_unitPath; }
    int inIndex() const { return m_inIndex; }
    int outIndex() const { return m_outIndex; }
    int slot() const { return m_slot; }

    // Derived gate for the Upload button: a target file plus a chosen In and
    // Out port row. Computed here so the condition has a single definition.
    bool canLoad() const {
        return !m_unitPath.isEmpty() && m_inIndex >= 0 && m_outIndex >= 0;
    }

    void setLibraryIndex(int v);
    void setUnitPath(const QString &v);
    void setInIndex(int v);
    void setOutIndex(int v);
    void setSlot(int v);

signals:
    void libraryIndexChanged();
    void unitPathChanged();
    void inIndexChanged();
    void outIndexChanged();
    void slotChanged();
    void canLoadChanged();

private:
    int m_libraryIndex = -1;   // row into App.library (view-owned highlight)
    QString m_unitPath;        // upload target file (browse or library pick)
    int m_inIndex = -1;        // chosen row in App.inPorts
    int m_outIndex = -1;       // chosen row in App.outPorts
    int m_slot = -1;           // -1 = auto (no -s); 0..15 = explicit slot
};

} // namespace qlogue

#endif // VIEWSTATE_H
