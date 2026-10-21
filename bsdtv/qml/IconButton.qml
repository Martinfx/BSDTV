import QtQuick

Rectangle {
    id: button

    property var theme
    property real unit: 720
    property string symbol: ""
    property string label: ""
    readonly property bool highlighted: button.activeFocus

    signal clicked()

    width: button.unit * 0.062
    height: width
    radius: width / 2
    color: button.highlighted ? button.theme.focus : Qt.rgba(1, 1, 1, 0.08)
    activeFocusOnTab: true

    Accessible.role: Accessible.Button
    Accessible.name: button.label
    Accessible.onPressAction: button.clicked()

    Text {
        anchors.centerIn: parent
        text: button.symbol
        color: button.highlighted ? button.theme.background : button.theme.text
        font.pixelSize: button.unit * 0.03
    }

    TapHandler {
        onTapped: button.clicked()
    }

    Keys.onReturnPressed: button.clicked()
    Keys.onEnterPressed: button.clicked()
}
