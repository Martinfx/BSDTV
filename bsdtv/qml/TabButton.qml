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
    color: tab.highlighted ? tab.theme.focus : "transparent"
    activeFocusOnTab: true

    Accessible.role: Accessible.PageTab
    Accessible.name: tab.text

    Text {
        id: label
        anchors.centerIn: parent
        text: tab.text
        color: tab.highlighted ? tab.theme.background : (tab.selected ? tab.theme.text : tab.theme.subtext)
        font.pixelSize: tab.unit * 0.028
        font.weight: tab.selected ? Font.DemiBold : Font.Normal
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: tab.unit * 0.004
        width: label.implicitWidth * 0.6
        height: 3
        radius: 1.5
        visible: tab.selected && !tab.highlighted
        color: tab.theme.accent
    }

    TapHandler {
        onTapped: tab.forceActiveFocus()
    }

    Keys.onReturnPressed: tab.clicked()
    Keys.onEnterPressed: tab.clicked()
}
