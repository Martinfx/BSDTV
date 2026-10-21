import QtQuick
import BSDTV

// Screen saver after a while without input: slow photo slideshow with a big clock.
// Any key brings the home screen back.
FocusScope {
    id: ambient

    readonly property var photos: TV.ambientPhotos
    readonly property bool reducedMotion: TV.settings.reduced_motion === true
    readonly property real unit: ambient.height
    property int index: 0
    property bool frontVisible: true
    property date now: new Date()

    function next() {
        if (ambient.photos.length === 0)
            return
        ambient.index = (ambient.index + 1) % ambient.photos.length
        const target = ambient.frontVisible ? back : front
        target.source = ambient.photos[ambient.index]
        ambient.frontVisible = !ambient.frontVisible
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
    }

    Rectangle {
        anchors.fill: parent
        visible: ambient.photos.length === 0
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.darker(TV.theme.accent, 4) }
            GradientStop { position: 1.0; color: "black" }
        }
    }

    Image {
        id: front
        anchors.fill: parent
        source: ambient.photos.length > 0 ? ambient.photos[0] : ""
        sourceSize.width: width
        sourceSize.height: height
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        opacity: ambient.frontVisible ? 1 : 0

        Behavior on opacity {
            OpacityAnimator { duration: ambient.reducedMotion ? 0 : 1500 }
        }
    }

    Image {
        id: back
        anchors.fill: parent
        sourceSize.width: width
        sourceSize.height: height
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        opacity: ambient.frontVisible ? 0 : 1

        Behavior on opacity {
            OpacityAnimator { duration: ambient.reducedMotion ? 0 : 1500 }
        }
    }

    Timer {
        interval: 15000
        repeat: true
        running: ambient.visible && ambient.photos.length > 1
        onTriggered: ambient.next()
    }

    Timer {
        interval: 1000
        repeat: true
        running: ambient.visible
        onTriggered: ambient.now = new Date()
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: parent.height * 0.4
        gradient: Gradient {
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.7) }
        }
    }

    Column {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: ambient.width * 0.055
        anchors.bottomMargin: ambient.unit * 0.08

        Text {
            text: Qt.formatTime(ambient.now, TV.settings.clock_24h === false ? "h:mm AP" : "HH:mm")
            color: "white"
            font.pixelSize: ambient.unit * 0.13
            font.weight: Font.Light
        }
        Text {
            //: Date format for the clock, see QDate::toString().
            text: ambient.now.toLocaleDateString(Qt.locale(), qsTr("dddd d MMMM"))
            color: Qt.rgba(1, 1, 1, 0.85)
            font.pixelSize: ambient.unit * 0.035
        }
    }

    // Swallow the key that wakes the TV so it does not also act on the home screen.
    Keys.onPressed: event => {
        event.accepted = true
        TV.touch()
    }
}
