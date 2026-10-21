import QtQuick
import BSDTV

// Full-bleed banner at the top of the home screen, like the featured area of a
// smart TV: artwork on the right, a scrim on the left with the title.
FocusScope {
    id: hero

    property var slides: []
    property int index: 0
    property real unit: 720
    property real inset: 80
    property bool reducedMotion: false
    readonly property var theme: TV.theme
    readonly property var slide: hero.slides.length > 0 ? hero.slides[Math.min(hero.index, hero.slides.length - 1)] : null
    // The slide's most colourful brand colour: the background, or the logo colour on near-black banners.
    readonly property color brand: !hero.slide ? hero.theme.accent
        : Qt.color(hero.slide.color).hslLightness >= 0.2 ? hero.slide.color
        : Qt.color(hero.slide.textColor).hslSaturation > 0.4 ? hero.slide.textColor : Qt.lighter(hero.slide.color, 2.5)
    readonly property color base: hero.theme.background

    signal activated(var slide)

    function step(delta) {
        if (hero.slides.length > 0)
            hero.index = (hero.index + delta + hero.slides.length) % hero.slides.length
    }

    activeFocusOnTab: true
    clip: true
    Accessible.role: Accessible.Button
    Accessible.name: hero.slide ? hero.slide.name : ""

    Timer {
        interval: 9000
        repeat: true
        running: hero.visible && !hero.activeFocus && hero.slides.length > 1 && !hero.reducedMotion
        onTriggered: hero.step(1)
    }

    // Stand-in artwork when a slide has no image: the brand colour with soft rings.
    Rectangle {
        anchors.fill: parent
        visible: !artwork.visible
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.3; color: hero.base }
            GradientStop { position: 1.0; color: Qt.darker(hero.brand, 1.2) }
        }
    }

    Repeater {
        model: 3
        delegate: Rectangle {
            required property int index
            width: hero.height * (1.5 - index * 0.36)
            height: width
            radius: width / 2
            x: hero.width * 0.74 - width / 2
            y: hero.height * 0.5 - height / 2
            color: "transparent"
            border.width: hero.height * 0.045
            border.color: Qt.rgba(1, 1, 1, 0.05 + index * 0.03)
            visible: !artwork.visible
            Accessible.ignored: true
        }
    }

    Image {
        id: artwork
        anchors.fill: parent
        visible: status === Image.Ready
        source: hero.slide && hero.slide.image ? (hero.slide.image.indexOf("/") === 0 ? "file://" + hero.slide.image : hero.slide.image) : ""
        sourceSize.width: width
        sourceSize.height: height
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
    }

    // Scrims: readable text on the left, and a seamless fade into the rows below.
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: Qt.rgba(hero.base.r, hero.base.g, hero.base.b, 0.95) }
            GradientStop { position: 0.4; color: Qt.rgba(hero.base.r, hero.base.g, hero.base.b, 0.6) }
            GradientStop { position: 0.75; color: Qt.rgba(hero.base.r, hero.base.g, hero.base.b, 0.0) }
        }
    }
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(hero.base.r, hero.base.g, hero.base.b, 0.6) }
            GradientStop { position: 0.25; color: Qt.rgba(hero.base.r, hero.base.g, hero.base.b, 0.0) }
            GradientStop { position: 0.75; color: Qt.rgba(hero.base.r, hero.base.g, hero.base.b, 0.0) }
            GradientStop { position: 1.0; color: hero.base }
        }
    }

    Column {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: hero.inset
        anchors.bottomMargin: hero.height * 0.1
        width: hero.width * 0.5
        spacing: hero.unit * 0.01

        // The app above a custom title, like the provider logo above a film on a smart TV.
        Text {
            visible: hero.slide !== null && !!hero.slide.title
            text: hero.slide ? hero.slide.name : ""
            color: hero.brand
            font.pixelSize: hero.unit * 0.026
            font.weight: Font.Bold
        }
        Text {
            width: parent.width
            text: hero.slide ? (hero.slide.title || hero.slide.name) : ""
            color: hero.theme.text
            elide: Text.ElideRight
            font.pixelSize: hero.unit * 0.075
            font.weight: Font.DemiBold
        }
        Text {
            width: parent.width
            text: hero.slide ? qsTr("Featured") + "  |  " + hero.slide.tagline : ""
            color: hero.theme.text
            elide: Text.ElideRight
            font.pixelSize: hero.unit * 0.024
        }
        Item { width: 1; height: hero.unit * 0.006 }
        Rectangle {
            width: openLabel.implicitWidth + hero.unit * 0.05
            height: hero.unit * 0.054
            radius: height / 2
            color: hero.activeFocus ? hero.theme.focus : Qt.rgba(1, 1, 1, 0.16)

            Text {
                id: openLabel
                anchors.centerIn: parent
                text: "▶  " + qsTr("Open")
                color: hero.activeFocus ? hero.theme.background : hero.theme.text
                font.pixelSize: hero.unit * 0.022
                font.weight: Font.DemiBold
            }
        }
    }

    Row {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: hero.inset
        anchors.bottomMargin: hero.height * 0.12
        spacing: hero.unit * 0.01

        Repeater {
            model: hero.slides.length
            delegate: Rectangle {
                required property int index
                width: index === hero.index ? hero.unit * 0.03 : hero.unit * 0.01
                height: hero.unit * 0.01
                radius: height / 2
                color: index === hero.index ? hero.theme.text : Qt.rgba(1, 1, 1, 0.35)
                Accessible.ignored: true
            }
        }
    }

    Keys.onLeftPressed: hero.step(-1)
    Keys.onRightPressed: hero.step(1)
    Keys.onReturnPressed: if (hero.slide) hero.activated(hero.slide)
    Keys.onEnterPressed: if (hero.slide) hero.activated(hero.slide)
}
