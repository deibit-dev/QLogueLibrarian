import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GroupBox {
    title: "Load Unit"

    // The whole load form (target file, slot, chosen ports) is view state.
    // The Upload button passes it to the controller as arguments, so neither
    // the controller nor the model stores a selection.
    RowLayout {
        anchors.fill: parent
        spacing: 6

        TextField {
            id: unitField
            Layout.fillWidth: true
            placeholderText: "Select a .xxxunit file…"
            text: ViewState.unitPath
            onTextChanged: ViewState.unitPath = text
        }

        Button {
            text: "Browse…"
            onClicked: {
                var f = Dialogs.pickUnitFile()
                if (f !== "") {
                    ViewState.unitPath = f
                    // A browsed file is not a library row: clear the highlight.
                    ViewState.libraryIndex = -1
                }
            }
        }

        Label { text: "Slot:" }
        SpinBox {
            id: slotSpin
            from: 1
            to: 8
            value: ViewState.slot
            onValueChanged: ViewState.slot = value
            textFromValue: function(value) {
                return value === 0 ? "Auto" : value.toString()
            }
        }

        Button {
            text: "Upload"
            enabled: ViewState.canLoad
            onClicked: Controller.loadUnit(ViewState.unitPath, ViewState.slot,
                                           ViewState.inIndex, ViewState.outIndex)
        }
    }
}
