import QtQuick
import BSDTV

FocusScope {
    id: bar

    readonly property var theme: TV.theme
    property date now: new Date()
    readonly property int hour: bar.now.getHours()
    property real unit: 720

    signal settingsRequested()
    signal powerRequested()

    height: clockColumn.height

    Timer {
        interval: 1000
        running: bar.visible
        repeat: true
        onTriggered: bar.now = new Date()
    }

    Column {
        id: clockColumn
        anchors.left: parent.left
        spacing: 2

        Text {
            visible: TV.settings.greeting !== false
            color: bar.theme.subtext
            font.pixelSize: bar.unit * 0.028
            text: bar.hour < 5 ? qsTr("Good night")
                : bar.hour < 12 ? qsTr("Good morning")
                : bar.hour < 18 ? qsTr("Good afternoon")
                : qsTr("Good evening")
        }
        Text {
            color: bar.theme.text
            font.pixelSize: bar.unit * 0.075
            font.weight: Font.Light
            text: Qt.formatTime(bar.now, TV.settings.clock_24h === false ? "h:mm AP" : "HH:mm")
        }
        Text {
            color: bar.theme.subtext
            font.pixelSize: bar.unit * 0.024
            text: bar.now.toLocaleDateString(Qt.locale(), "dddd d MMMM")
        }
    }

    Row {
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: 14

        PillButton {
            id: networkPill
            theme: bar.theme
            text: TV.ipAddress !== "" ? TV.ipAddress : qsTr("Offline")
            symbol: TV.ipAddress !== "" ? "●" : "○"
            KeyNavigation.right: settingsPill
            onClicked: bar.settingsRequested()
        }
        PillButton {
            id: settingsPill
            theme: bar.theme
            text: qsTr("Settings")
            symbol: "⚙"
            focus: true
            KeyNavigation.left: networkPill
            KeyNavigation.right: powerPill
            onClicked: bar.settingsRequested()
        }
        PillButton {
            id: powerPill
            theme: bar.theme
            text: qsTr("Power")
            symbol: "⏻"
            KeyNavigation.left: settingsPill
            onClicked: bar.powerRequested()
        }
    }
}
