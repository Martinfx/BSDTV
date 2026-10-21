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
    signal searchRequested()
    signal profileRequested()

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

        Item {
            id: profileButton
            width: forYouTab.height
            height: forYouTab.height
            activeFocusOnTab: true
            KeyNavigation.right: forYouTab
            Accessible.role: Accessible.Button
            Accessible.name: TV.profile.name
            // Exposed for tests and accessibility tools.
            readonly property string label: TV.profile.name

            Avatar {
                anchors.centerIn: parent
                width: parent.height * (profileButton.activeFocus ? 1.0 : 0.86)
                height: width
                name: TV.profile.name
                tint: TV.profile.color
                highlighted: profileButton.activeFocus
            }

            Keys.onReturnPressed: bar.profileRequested()
            Keys.onEnterPressed: bar.profileRequested()
        }

        TabButton {
            id: forYouTab
            theme: bar.theme
            unit: bar.unit
            text: qsTr("For you")
            selected: bar.currentTab === 0
            focus: true
            KeyNavigation.left: profileButton
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
            KeyNavigation.right: searchButton
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
            id: searchButton
            theme: bar.theme
            unit: bar.unit
            label: qsTr("Search")
            KeyNavigation.left: appsTab
            KeyNavigation.right: settingsButton
            onClicked: bar.searchRequested()

            SearchIcon {
                anchors.centerIn: parent
                width: parent.width * 0.42
                height: width
                tint: searchButton.highlighted ? bar.theme.background : bar.theme.text
            }
        }
        IconButton {
            id: settingsButton
            theme: bar.theme
            unit: bar.unit
            symbol: "⚙"
            label: qsTr("Settings")
            KeyNavigation.left: searchButton
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
