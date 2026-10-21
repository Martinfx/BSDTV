import QtQuick

// Round profile picture: the first letter of the name on the profile colour.
Rectangle {
    id: avatar

    property string name: ""
    property color tint: "#8ab4f8"
    property bool highlighted: false

    radius: width / 2
    color: avatar.tint
    border.width: avatar.highlighted ? Math.max(2, width * 0.05) : 0
    border.color: "white"

    Text {
        anchors.centerIn: parent
        text: avatar.name.length > 0 ? avatar.name.charAt(0).toUpperCase() : "?"
        color: Qt.color(avatar.tint).hslLightness > 0.6 ? "#202124" : "white"
        font.pixelSize: avatar.height * 0.46
        font.weight: Font.DemiBold
    }
}
