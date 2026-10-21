import QtQuick

Rectangle {
    id: pill

    property var theme
    property string text: ""
    property string symbol: ""
    readonly property bool highlighted: pill.activeFocus

    signal clicked()

    implicitWidth: label.implicitWidth + 40
    implicitHeight: label.implicitHeight + 20
    radius: height / 2
    color: pill.highlighted ? pill.theme.focus : Qt.rgba(1, 1, 1, 0.08)
    border.width: 1
    border.color: Qt.rgba(1, 1, 1, pill.highlighted ? 0 : 0.14)
    activeFocusOnTab: true

    Accessible.role: Accessible.Button
    Accessible.name: pill.text
    Accessible.onPressAction: pill.clicked()

    Text {
        id: label
        anchors.centerIn: parent
        text: pill.symbol !== "" ? pill.symbol + "  " + pill.text : pill.text
        color: pill.highlighted ? pill.theme.background : pill.theme.text
        font.pixelSize: 20
    }

    TapHandler {
        onTapped: pill.clicked()
    }

    Keys.onReturnPressed: pill.clicked()
    Keys.onEnterPressed: pill.clicked()
}
