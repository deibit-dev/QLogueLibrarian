import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    title: "QLogueLibrarian"
    width: 780
    height: 560
    visible: true

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.margins: 4

            Label { text: "logue-cli:" }
            TextField {
                id: cliPathField
                Layout.fillWidth: true
                text: App.cliPath
                // The model is read-only for the view: the user's edit is
                // committed as a command on focus-out/Enter, never written
                // to Logic directly.
                onEditingFinished: Controller.setCliPath(text)
            }
            Button {
                text: "..."
                onClicked: {
                    var f = Dialogs.pickExecutable()
                    if (f !== "")
                        Controller.setCliPath(f)
                }
            }
        }
    }

    footer: ToolBar {
        Label {
            anchors.fill: parent
            anchors.margins: 4
            text: App.statusText
            elide: Text.ElideRight
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        MidiSection   { Layout.fillWidth: true }
        LoadSection   { Layout.fillWidth: true }
        UnitLibrary { Layout.fillWidth: true; Layout.fillHeight: true }
        LogPanel      { Layout.fillWidth: true; Layout.preferredHeight: 120 }
    }

    // keep cliPath field in sync when the controller persists a new path
    // (e.g. from the Browse dialog), in case a previous edit broke the binding
    Connections {
        target: App
        function onCliPathChanged() { cliPathField.text = App.cliPath }
    }
}
