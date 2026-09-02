#include "ViewState.h"

namespace qlogue {

ViewState::ViewState(QObject *parent)
    : QObject(parent)
{
}

// Each setter keeps its own signal and, when the change affects whether a
// load is possible, re-emits canLoadChanged so the Upload button stays in
// sync. Guards avoid redundant signals on no-op writes (the QML text fields
// write back the value they just read, which must not loop).
void ViewState::setLibraryIndex(int v) {
    if (m_libraryIndex == v) return;
    m_libraryIndex = v;
    emit libraryIndexChanged();
}

void ViewState::setUnitPath(const QString &v) {
    if (m_unitPath == v) return;
    m_unitPath = v;
    emit unitPathChanged();
    emit canLoadChanged();
}

void ViewState::setInIndex(int v) {
    if (m_inIndex == v) return;
    m_inIndex = v;
    emit inIndexChanged();
    emit canLoadChanged();
}

void ViewState::setOutIndex(int v) {
    if (m_outIndex == v) return;
    m_outIndex = v;
    emit outIndexChanged();
    emit canLoadChanged();
}

void ViewState::setSlot(int v) {
    if (m_slot == v) return;
    m_slot = v;
    emit slotChanged();
}

} // namespace qlogue
