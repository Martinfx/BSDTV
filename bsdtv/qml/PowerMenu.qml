import QtQuick
import BSDTV

FocusScope {
    id: menu

    readonly property var theme: TV.theme

    signal closed()

    function run(action) {
        menu.closed()
        TV.power(action)
    }

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.6)
        Accessible.ignored: true
    }

    Rectangle {
        anchors.centerIn: content
        width: content.width + 64
        height: content.height + 56
        radius: 28
        color: menu.theme.surface
    }

    Column {
        id: content
        anchors.centerIn: parent
        spacing: 18

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Power")
            color: menu.theme.text
            font.pixelSize: 36
            font.weight: Font.DemiBold
            bottomPadding: 12
        }

        Row {
            spacing: 18

            PillButton {
                id: sleepButton
                theme: menu.theme
                text: qsTr("Screen off")
                focus: true
                KeyNavigation.right: restartLauncherButton
                onClicked: menu.run("display_off")
            }
            PillButton {
                id: restartLauncherButton
                theme: menu.theme
                text: qsTr("Restart home screen")
                KeyNavigation.left: sleepButton
                KeyNavigation.right: rebootButton
                onClicked: menu.run("restart_launcher")
            }
            PillButton {
                id: rebootButton
                theme: menu.theme
                text: qsTr("Reboot")
                KeyNavigation.left: restartLauncherButton
                KeyNavigation.right: poweroffButton
                onClicked: menu.run("reboot")
            }
            PillButton {
                id: poweroffButton
                theme: menu.theme
                text: qsTr("Shut down")
                KeyNavigation.left: rebootButton
                KeyNavigation.right: cancelButton
                onClicked: menu.run("poweroff")
            }
            PillButton {
                id: cancelButton
                theme: menu.theme
                text: qsTr("Cancel")
                KeyNavigation.left: poweroffButton
                onClicked: menu.closed()
            }
        }
    }

    Keys.onEscapePressed: menu.closed()
    Keys.onBackPressed: menu.closed()
}
