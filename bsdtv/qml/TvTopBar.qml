import QtQuick
import BSDTV

FocusScope {
    id: bar

    readonly property var theme: TV.theme
    property int currentTab: 0
    property real unit: 720
    property date now: new Date()

    signal settingsRequested()
    signal powerRequested()

    height: tabs.height

    Timer {
        interval: 1000
        running: bar.visible
        repeat: true
        onTriggered: bar.now = new Date()
    }

    Row {
        id: tabs
        anchors.left: parent.left
        spacing: bar.unit * 0.012

        TabButton {
            id: forYouTab
            theme: bar.theme
            unit: bar.unit
            text: qsTr("For you")
            selected: bar.currentTab === 0
            focus: true
            KeyNavigation.right: appsTab
            onActiveFocusChanged: if (activeFocus) bar.currentTab = 0
        }
        TabButton {
            id: appsTab
            theme: bar.theme
            unit: bar.unit
            text: qsTr("Apps")
            selected: bar.currentTab === 1
            KeyNavigation.left: forYouTab
            KeyNavigation.right: settingsButton
            onActiveFocusChanged: if (activeFocus) bar.currentTab = 1
        }
    }

    Row {
        anchors.right: parent.right
        anchors.verticalCenter: tabs.verticalCenter
        spacing: bar.unit * 0.016

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: Qt.formatTime(bar.now, TV.settings.clock_24h === false ? "h:mm AP" : "HH:mm")
            color: bar.theme.text
            font.pixelSize: bar.unit * 0.032
            rightPadding: bar.unit * 0.01
        }
        IconButton {
            id: settingsButton
            theme: bar.theme
            unit: bar.unit
            symbol: "⚙"
            label: qsTr("Settings")
            KeyNavigation.left: appsTab
            KeyNavigation.right: powerButton
            onClicked: bar.settingsRequested()
        }
        IconButton {
            id: powerButton
            theme: bar.theme
            unit: bar.unit
            symbol: "⏻"
            label: qsTr("Power")
            KeyNavigation.left: settingsButton
            onClicked: bar.powerRequested()
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: "BSDTV"
            color: bar.theme.text
            font.pixelSize: bar.unit * 0.026
            font.weight: Font.DemiBold
            leftPadding: bar.unit * 0.006
            Accessible.ignored: true
        }
    }
}
