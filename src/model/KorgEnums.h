#ifndef KORGENUMS_H
#define KORGENUMS_H

#include <QString>
#include <QStringList>

namespace qlogue {

/// File extension globs that logue-cli recognises (for directory scanning).
inline QStringList unitExtensions() {
    return {
        QStringLiteral("*.prlgunit"),
        QStringLiteral("*.mnlgxdunit"),
        QStringLiteral("*.ntkdigunit"),
        QStringLiteral("*.drmlgunit"),
        QStringLiteral("*.nts1mkiiunit"),
        QStringLiteral("*.nts3unit"),
        QStringLiteral("*.mkg2unit"),
        QStringLiteral("*.unit"),
    };
}

/// File filter string for QFileDialog (used by the view's Dialogs helper).
inline QString unitFileFilter() {
    return QStringLiteral(
        "All logue units (*.prlgunit *.mnlgxdunit *.ntkdigunit "
        "*.nts1mkiiunit *.nts3unit *.drmlgunit *.mkg2unit *.unit);;"
        "prologue (*.prlgunit);;"
        "minilogue xd (*.mnlgxdunit);;"
        "NTS-1 (*.ntkdigunit);;"
        "NTS-1 mkII (*.nts1mkiiunit);;"
        "NTS-3 (*.nts3unit);;"
        "drumlogue (*.drmlgunit);;"
        "microKORG2 (*.mkg2unit)");
}

} // namespace qlogue

#endif // KORGENUMS_H
