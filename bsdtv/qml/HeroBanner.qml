import QtQuick
import BSDTV

FocusScope {
    id: hero

    property var slides: []
    property int index: 0
    property real unit: 720
    property bool reducedMotion: false
    readonly property var theme: TV.theme
    readonly property var slide: hero.slides.length > 0 ? hero.slides[Math.min(hero.index, hero.slides.length - 1)] : null

    signal launched(string appId)

    function step(delta) {
        if (hero.slides.length > 0)
            hero.index = (hero.index + delta + hero.slides.length) % hero.slides.length
    }

    activeFocusOnTab: true
    Accessible.role: Accessible.Button
    Accessible.name: hero.slide ? hero.slide.name : ""

    Timer {
        interval: 9000
        repeat: true
        running: hero.visible && !hero.activeFocus && hero.slides.length > 1 && !hero.reducedMotion
        onTriggered: hero.step(1)
    }

    Rectangle {
        id: card
        anchors.fill: parent
        radius: hero.unit * 0.03
        clip: true
        color: hero.theme.surface
        border.width: hero.activeFocus ? 4 : 0
        border.color: hero.theme.focus

        Rectangle {
            anchors.fill: parent
            anchors.margins: card.border.width
            radius: card.radius
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Qt.darker(hero.slide ? hero.slide.color : hero.theme.accent, 3.2) }
                GradientStop { position: 0.55; color: Qt.darker(hero.slide ? hero.slide.color : hero.theme.accent, 1.6) }
                GradientStop { position: 1.0; color: hero.slide ? hero.slide.color : hero.theme.accent }
            }
        }

        // Large soft rings on the right, a stand-in for artwork when a slide has no image.
        Repeater {
            model: 3
            delegate: Rectangle {
                required property int index
                width: card.height * (1.6 - index * 0.38)
                height: width
                radius: width / 2
                x: card.width * 0.72 - width / 2
                y: card.height * 0.5 - height / 2
                color: "transparent"
                border.width: card.height * 0.05
                border.color: Qt.rgba(1, 1, 1, 0.06 + index * 0.03)
                visible: !artwork.visible
                Accessible.ignored: true
            }
        }

        Image {
            id: artwork
            anchors.fill: parent
            anchors.margins: card.border.width
            visible: status === Image.Ready
            source: hero.slide && hero.slide.image ? (hero.slide.image.indexOf("/") === 0 ? "file://" + hero.slide.image : hero.slide.image) : ""
            sourceSize.width: width
            sourceSize.height: height
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: card.border.width
            visible: artwork.visible
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, 0.85) }
                GradientStop { position: 0.6; color: Qt.rgba(0, 0, 0, 0.1) }
            }
        }

        Column {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: card.width * 0.05
            width: card.width * 0.5
            spacing: hero.unit * 0.012

            Text {
                text: qsTr("Featured")
                color: Qt.rgba(1, 1, 1, 0.7)
                font.pixelSize: hero.unit * 0.02
                font.weight: Font.DemiBold
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 1.5
            }
            Text {
                width: parent.width
                text: hero.slide ? hero.slide.name : ""
                color: "white"
                elide: Text.ElideRight
                font.pixelSize: hero.unit * 0.075
                font.weight: Font.Bold
            }
            Text {
                width: parent.width
                text: hero.slide ? hero.slide.tagline : ""
                color: Qt.rgba(1, 1, 1, 0.85)
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
                font.pixelSize: hero.unit * 0.028
            }
            Item { width: 1; height: hero.unit * 0.01 }
            Rectangle {
                width: openLabel.implicitWidth + hero.unit * 0.05
                height: hero.unit * 0.058
                radius: height / 2
                color: hero.activeFocus ? "white" : Qt.rgba(1, 1, 1, 0.18)

                Text {
                    id: openLabel
                    anchors.centerIn: parent
                    text: "▶  " + qsTr("Open")
                    color: hero.activeFocus ? "#111" : "white"
                    font.pixelSize: hero.unit * 0.024
                    font.weight: Font.DemiBold
                }
            }
        }

        Row {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: card.height * 0.08
            spacing: hero.unit * 0.01

            Repeater {
                model: hero.slides.length
                delegate: Rectangle {
                    required property int index
                    width: index === hero.index ? hero.unit * 0.03 : hero.unit * 0.01
                    height: hero.unit * 0.01
                    radius: height / 2
                    color: index === hero.index ? "white" : Qt.rgba(1, 1, 1, 0.4)
                    Accessible.ignored: true
                }
            }
        }
    }

    Keys.onLeftPressed: hero.step(-1)
    Keys.onRightPressed: hero.step(1)
    Keys.onReturnPressed: if (hero.slide) hero.launched(hero.slide.appId)
    Keys.onEnterPressed: if (hero.slide) hero.launched(hero.slide.appId)
}
