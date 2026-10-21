import QtQuick

Rectangle {
    id: row

    property var theme
    property string label: ""
    property string value: ""
    property bool adjustable: true
    readonly property bool highlighted: row.activeFocus

    signal decrease()
    signal increase()

    width: parent ? parent.width : 400
    height: 64
    radius: 14
    color: row.highlighted ? Qt.rgba(1, 1, 1, 0.14) : "transparent"
    activeFocusOnTab: true

    Accessible.role: Accessible.ListItem
    Accessible.name: row.label + ": " + row.value

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.verticalCenter: parent.verticalCenter
        text: row.label
        color: row.theme.text
        font.pixelSize: 22
    }

    Text {
        anchors.right: parent.right
        anchors.rightMargin: 20
        anchors.verticalCenter: parent.verticalCenter
        text: row.adjustable && row.highlighted ? "‹  " + row.value + "  ›" : row.value
        color: row.highlighted ? row.theme.accent : row.theme.subtext
        font.pixelSize: 22
    }

    Keys.onLeftPressed: if (row.adjustable) row.decrease()
    Keys.onRightPressed: if (row.adjustable) row.increase()
    Keys.onReturnPressed: if (row.adjustable) row.increase()
    Keys.onEnterPressed: if (row.adjustable) row.increase()
}
