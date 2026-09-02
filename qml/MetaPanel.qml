import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    id: metaRoot

    // The inspected unit is derived from the view's current library selection.
    // Reading App.library[i] keeps the model as the single source of truth:
    // nothing is copied into the model or this panel.
    property var unit: {
        var i = ViewState.libraryIndex
        if (i < 0 || i >= App.library.length)
            return null
        return App.library[i]
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        GridLayout {
            columns: 2
            columnSpacing: 12
            rowSpacing: 4
            Layout.fillWidth: true

            Label { text: "Name:";     font.bold: true; Layout.alignment: Qt.AlignRight }
            Label {
                Layout.fillWidth: true; wrapMode: Text.Wrap
                text: metaRoot.unit
                      ? (metaRoot.unit.name.trim() !== ""
                         ? metaRoot.unit.name.trim() : metaRoot.unit.fileName)
                      : "—"
            }

            Label { text: "Platform:"; font.bold: true; Layout.alignment: Qt.AlignRight }
            Label { text: metaRoot.unit ? metaRoot.unit.platform : "—"; Layout.fillWidth: true }

            Label { text: "Module:";   font.bold: true; Layout.alignment: Qt.AlignRight }
            Label { text: metaRoot.unit ? metaRoot.unit.module : "—";   Layout.fillWidth: true }

            Label { text: "Version:";  font.bold: true; Layout.alignment: Qt.AlignRight }
            Label { text: metaRoot.unit ? metaRoot.unit.version : "—";  Layout.fillWidth: true }

            Label { text: "API:";      font.bold: true; Layout.alignment: Qt.AlignRight }
            Label { text: metaRoot.unit ? metaRoot.unit.api : "—";      Layout.fillWidth: true }

            Label { text: "Dev ID:";   font.bold: true; Layout.alignment: Qt.AlignRight }
            Label { text: metaRoot.unit ? metaRoot.unit.devId : "—";    Layout.fillWidth: true }

            Label { text: "Prg ID:";   font.bold: true; Layout.alignment: Qt.AlignRight }
            Label { text: metaRoot.unit ? metaRoot.unit.prgId : "—";    Layout.fillWidth: true }

            Label { text: "Params:";   font.bold: true; Layout.alignment: Qt.AlignRight }
            Label { text: metaRoot.unit ? metaRoot.unit.numParams : "—"; Layout.fillWidth: true }
        }

        // ── Parameter table ──────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "transparent"
            border.color: palette.mid
            border.width: 1
            radius: 2

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 1
                spacing: 0

                // header row
                Rectangle {
                    Layout.fillWidth: true
                    height: 28
                    color: palette.alternateBase

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 6
                        anchors.rightMargin: 6
                        spacing: 4

                        Label { text: "Name"; font.bold: true; Layout.fillWidth: true; Layout.preferredWidth: 120 }
                        Label { text: "Min";  font.bold: true; Layout.preferredWidth: 50; horizontalAlignment: Text.AlignHCenter }
                        Label { text: "Max";  font.bold: true; Layout.preferredWidth: 50; horizontalAlignment: Text.AlignHCenter }
                        Label { text: "Type"; font.bold: true; Layout.preferredWidth: 50; horizontalAlignment: Text.AlignHCenter }
                    }
                }

                // rows
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: metaRoot.unit ? metaRoot.unit.params : []

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 26
                        color: index % 2 === 0 ? "transparent" : palette.alternateBase

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 6
                            spacing: 4

                            Label { text: modelData.name; Layout.fillWidth: true; Layout.preferredWidth: 120; elide: Text.ElideRight }
                            Label { text: modelData.min;  Layout.preferredWidth: 50; horizontalAlignment: Text.AlignHCenter }
                            Label { text: modelData.max;  Layout.preferredWidth: 50; horizontalAlignment: Text.AlignHCenter }
                            Label { text: modelData.type; Layout.preferredWidth: 50; horizontalAlignment: Text.AlignHCenter }
                        }
                    }

                    ScrollBar.vertical: ScrollBar {}
                }
            }
        }
    }
}
