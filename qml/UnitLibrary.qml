import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    title: "Unit Library"

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        // directory picker
        RowLayout {
            spacing: 6
            Label { text: "Directory:" }
            TextField {
                id: dirField
                Layout.fillWidth: true
                placeholderText: "Select unit directory…"
                text: App.unitDir
                // Model is read-only for the view: commit as a command on
                // Enter/focus-out (which also triggers the rescan).
                onEditingFinished: Controller.setUnitDir(text)
            }
            Button {
                text: "Browse…"
                onClicked: {
                    var d = Dialogs.pickDirectory(App.unitDir)
                    if (d !== "")
                        Controller.setUnitDir(d)
                }
            }
        }

        // splitter: list (left) + meta panel (right)
        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            ListView {
                id: unitList
                SplitView.preferredWidth: 200
                SplitView.minimumWidth: 140
                clip: true
                model: App.library
                currentIndex: ViewState.libraryIndex

                delegate: ItemDelegate {
                    width: unitList.width
                    // Presentation fallback: prefer the manifest name, fall
                    // back to the file name when the manifest has none.
                    text: modelData.isValid && modelData.name.trim() !== ""
                          ? modelData.name.trim() : modelData.fileName
                    highlighted: ListView.isCurrentItem
                    opacity: modelData.isValid ? 1.0 : 0.5
                    onClicked: {
                        ViewState.libraryIndex = index
                        ViewState.unitPath = modelData.filePath
                    }
                }

                ScrollBar.vertical: ScrollBar {}
            }

            MetaPanel {
                SplitView.fillWidth: true
            }
        }
    }

    Connections {
        target: App
        function onUnitDirChanged() { dirField.text = App.unitDir }
        // The library was replaced wholesale: drop the highlight so the meta
        // panel no longer points at a stale row.
        function onLibraryChanged()   { ViewState.libraryIndex = -1 }
    }
}
