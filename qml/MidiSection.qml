import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    title: "MIDI Ports"

    // Choosing which port is active is view state (ViewState), not model
    // state. `defaultPortIndex` reproduces the old auto-select: prefer the
    // device whose name contains "SOUND", otherwise the first port.
    function defaultPortIndex(labels) {
        for (var i = 0; i < labels.length; i++)
            if (labels[i].toLowerCase().indexOf("sound") >= 0)
                return i
        return labels.length > 0 ? 0 : -1
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        Button {
            text: "Probe"
            onClicked: Controller.probe()
        }

        RowLayout {
            spacing: 8
            Label { text: "In:" }
            ComboBox {
                id: inCombo
                Layout.fillWidth: true
                model: App.inPorts
                onCurrentIndexChanged: ViewState.inIndex = currentIndex
            }
            Label { text: "Out:" }
            ComboBox {
                id: outCombo
                Layout.fillWidth: true
                model: App.outPorts
                onCurrentIndexChanged: ViewState.outIndex = currentIndex
            }
        }
    }

    // One-way sync: ViewState owns the selected port and the combos follow
    // it. A `currentIndex: ViewState.inIndex` binding is NOT used because the
    // ComboBox rewrites currentIndex when its model is reset on a fresh
    // probe, which would break the binding.
    Connections {
        target: ViewState
        function onInIndexChanged() {
            if (inCombo.currentIndex !== ViewState.inIndex)
                inCombo.currentIndex = ViewState.inIndex
        }
        function onOutIndexChanged() {
            if (outCombo.currentIndex !== ViewState.outIndex)
                outCombo.currentIndex = ViewState.outIndex
        }
    }

    // A fresh probe always re-defaults to the SOUND port (same behaviour as
    // the old autoSelect). callLater defers this until after the ComboBox
    // model reset has settled, so the reset's own currentIndex write-back
    // cannot clobber the auto-selected default.
    Connections {
        target: App
        function onInPortsChanged() {
            Qt.callLater(function() { ViewState.inIndex = defaultPortIndex(App.inPorts) })
        }
        function onOutPortsChanged() {
            Qt.callLater(function() { ViewState.outIndex = defaultPortIndex(App.outPorts) })
        }
    }
}
