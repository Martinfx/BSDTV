import QtQuick
import BSDTV

// "Who's watching?": pick a profile with the arrows and OK.
FocusScope {
    id: picker

    readonly property var theme: TV.theme
    readonly property real unit: picker.height

    signal closed()

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.85)
        Accessible.ignored: true
    }

    Column {
        anchors.centerIn: parent
        spacing: picker.unit * 0.06

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Who's watching?")
            color: picker.theme.text
            font.pixelSize: picker.unit * 0.06
            font.weight: Font.DemiBold
        }

        ListView {
            id: list
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(picker.width * 0.9, count * (picker.unit * 0.2 + spacing))
            height: picker.unit * 0.28
            orientation: ListView.Horizontal
            spacing: picker.unit * 0.04
            interactive: false
            focus: true
            keyNavigationEnabled: true
            model: TV.profiles
            currentIndex: Math.max(0, TV.profiles.findIndex(p => p.id === TV.profile.id))

            delegate: Column {
                id: entry

                required property var modelData
                required property int index
                readonly property bool current: ListView.isCurrentItem
                readonly property string name: entry.modelData.name

                spacing: picker.unit * 0.02
                width: picker.unit * 0.2

                Item {
                    width: parent.width
                    height: picker.unit * 0.19

                    Avatar {
                        anchors.centerIn: parent
                        width: picker.unit * (entry.current ? 0.19 : 0.16)
                        height: width
                        name: entry.modelData.name
                        tint: entry.modelData.color
                        highlighted: entry.current
                    }
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: entry.modelData.name
                    color: entry.current ? picker.theme.text : picker.theme.subtext
                    font.pixelSize: picker.unit * 0.03
                }
            }

            Keys.onReturnPressed: {
                TV.setProfile(TV.profiles[list.currentIndex].id)
                picker.closed()
            }
            Keys.onEnterPressed: {
                TV.setProfile(TV.profiles[list.currentIndex].id)
                picker.closed()
            }
        }
    }

    Keys.onEscapePressed: picker.closed()
    Keys.onBackPressed: picker.closed()
}
