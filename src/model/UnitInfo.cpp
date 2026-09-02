#include "UnitInfo.h"

#include <QVariant>

namespace qlogue {

// Projects the internal QVector<UnitParam> as a QVariantList of UnitParam
// value types. Done on demand (not cached) so m_params stays the single
// canonical storage and no second container is kept in sync.
QVariantList UnitInfo::params() const {
    QVariantList list;
    list.reserve(m_params.size());
    for (const UnitParam &p : m_params)
        list.append(QVariant::fromValue(p));
    return list;
}

} // namespace qlogue
