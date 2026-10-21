import QtQuick

Item {
    id: tile

    required property string appId
    required property string name
    required property string caption
    required property string color
    required property string icon
    required property string textColor
    property bool highlighted: false
    property bool running: false
    property bool reducedMotion: false
    property var theme
    property bool round: false
    readonly property bool flat: tile.theme.tiles === "flat"
    // Brand banners are often near-black with a coloured logo; glossy tiles use the logo colour instead.
    readonly property color vivid: Qt.color(tile.color).hslLightness >= 0.2 ? tile.color
        : Qt.color(tile.textColor).hslSaturation > 0.4 ? tile.textColor : Qt.lighter(tile.color, 2.5)

    Accessible.role: Accessible.Button
    Accessible.name: tile.name

    Rectangle {
        id: body
        anchors.fill: parent
        radius: tile.round ? height / 2 : height * (tile.flat ? 0.08 : 0.12)
        scale: tile.highlighted ? (tile.flat ? 1.1 : 1.08) : 1.0
        // Flat near-black tiles get a hairline so they do not melt into the background.
        border.width: tile.highlighted ? 3 : (tile.flat ? 1 : 0)
        border.color: tile.highlighted ? tile.theme.focus : Qt.rgba(1, 1, 1, 0.1)
        color: tile.color
        gradient: tile.flat ? null : glossy

        Gradient {
            id: glossy
            GradientStop { position: 0.0; color: Qt.lighter(tile.vivid, 1.25) }
            GradientStop { position: 1.0; color: Qt.darker(tile.vivid, 1.6) }
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
            width: parent.width * (tile.round ? 0.82 : 0.9)
            visible: !iconImage.visible
            text: tile.name
            color: tile.flat ? tile.textColor : "white"
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            font.pixelSize: parent.height * (tile.round ? 0.17 : 0.22)
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: 8
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
