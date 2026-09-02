#ifndef UNITHEADERPARSER_H
#define UNITHEADERPARSER_H

#include "model/UnitInfo.h"

namespace qlogue {

/// Extracts the manifest.json from a .xxxunit ZIP archive (via miniz)
/// and populates a UnitInfo struct from the JSON contents.
class UnitHeaderParser
{
public:
    /// Parse the manifest.json inside the ZIP at @p filePath.
    /// Returns a UnitInfo with isValid=true on success.
    static UnitInfo parse(const QString &filePath);

private:
    UnitHeaderParser() = delete;
};

} // namespace qlogue

#endif // UNITHEADERPARSER_H
