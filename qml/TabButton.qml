import QtQuick

Rectangle {
    id: tab

    property var theme
    property real unit: 720
    property string text: ""
    property bool selected: false
    readonly property bool highlighted: tab.activeFocus

    signal clicked()

    implicitWidth: label.implicitWidth + tab.unit * 0.05
    implicitHeight: tab.unit * 0.062
    radius: height / 2
    color: tab.selected || tab.highlighted ? tab.theme.focus : "transparent"
    border.width: tab.highlighted ? 3 : 0
    border.color: tab.theme.accent
    activeFocusOnTab: true

    Accessible.role: Accessible.PageTab
    Accessible.name: tab.text

    Text {
        id: label
        anchors.centerIn: parent
        text: tab.text
        color: tab.selected || tab.highlighted ? tab.theme.background : tab.theme.text
        font.pixelSize: tab.unit * 0.026
        font.weight: Font.DemiBold
    }

    TapHandler {
        onTapped: tab.forceActiveFocus()
    }

    Keys.onReturnPressed: tab.clicked()
    Keys.onEnterPressed: tab.clicked()
}
