import QtQuick

Item {
    id: tile

    required property string appId
    required property string name
    required property string caption
    required property string color
    required property string icon
    property bool highlighted: false
    property bool running: false
    property bool reducedMotion: false
    property var theme

    Accessible.role: Accessible.Button
    Accessible.name: tile.name

    Rectangle {
        id: body
        anchors.fill: parent
        radius: height * 0.12
        scale: tile.highlighted ? 1.08 : 1.0
        border.width: tile.highlighted ? 3 : 0
        border.color: tile.theme.focus
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.lighter(tile.color, 1.25) }
            GradientStop { position: 1.0; color: Qt.darker(tile.color, 1.6) }
        }

        Behavior on scale {
            enabled: !tile.reducedMotion
            NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
        }

        Image {
            id: iconImage
            anchors.centerIn: parent
            width: parent.width * 0.4
            height: parent.height * 0.5
            visible: tile.icon !== "" && status === Image.Ready
            source: tile.icon ? (tile.icon.indexOf("/") === 0 ? "file://" + tile.icon : tile.icon) : ""
            sourceSize.width: width
            sourceSize.height: height
            fillMode: Image.PreserveAspectFit
            asynchronous: true
        }

        Text {
            anchors.centerIn: parent
            width: parent.width * 0.9
            visible: !iconImage.visible
            text: tile.name
            color: "white"
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            font.pixelSize: parent.height * 0.22
            font.weight: Font.Bold
        }

        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: parent.height * 0.08
            width: parent.height * 0.08
            height: width
            radius: width / 2
            visible: tile.running
            color: "#3ddc84"
            Accessible.ignored: true
        }
    }
}
