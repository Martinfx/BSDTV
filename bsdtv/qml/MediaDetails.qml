import QtQuick
import BSDTV

// Details of a film or episode, with "Watch on" buttons like a smart TV.
FocusScope {
    id: details

    required property var item
    readonly property var theme: TV.theme
    readonly property real unit: details.height
    readonly property color base: details.theme.background

    signal closed()

    function minutesText(minutes) {
        return minutes >= 60 ? qsTr("%1 h %2 min").arg(Math.floor(minutes / 60)).arg(minutes % 60)
                             : qsTr("%1 min").arg(minutes)
    }

    Rectangle {
        anchors.fill: parent
        color: details.base
    }

    Image {
        id: backdrop
        anchors.fill: parent
        source: details.item.backdrop || details.item.image
        sourceSize.width: width
        sourceSize.height: height
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: Qt.rgba(details.base.r, details.base.g, details.base.b, 0.97) }
            GradientStop { position: 0.45; color: Qt.rgba(details.base.r, details.base.g, details.base.b, 0.75) }
            GradientStop { position: 0.8; color: Qt.rgba(details.base.r, details.base.g, details.base.b, 0.1) }
        }
    }

    Column {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: details.width * 0.055
        width: details.width * 0.5
        spacing: details.unit * 0.018

        Text {
            width: parent.width
            text: details.item.title
            color: details.theme.text
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
            font.pixelSize: details.unit * 0.075
            font.weight: Font.DemiBold
        }
        Text {
            width: parent.width
            text: [details.item.type === "Episode" ? details.item.subtitle : "",
                   details.item.year > 0 ? String(details.item.year) : "",
                   details.item.minutes > 0 ? details.minutesText(details.item.minutes) : "",
                   details.item.rating].filter(s => s !== "").join("  ·  ")
            color: details.theme.subtext
            elide: Text.ElideRight
            font.pixelSize: details.unit * 0.026
        }
        Text {
            width: parent.width
            text: details.item.overview
            color: details.theme.text
            wrapMode: Text.WordWrap
            maximumLineCount: 5
            elide: Text.ElideRight
            lineHeight: 1.15
            font.pixelSize: details.unit * 0.025
        }
        Text {
            visible: details.item.progress > 0
            text: qsTr("%1 left").arg(details.minutesText(Math.max(1, Math.round(details.item.minutes * (1 - details.item.progress)))))
            color: details.theme.accent
            font.pixelSize: details.unit * 0.024
        }

        Item { width: 1; height: details.unit * 0.01 }

        Text {
            text: qsTr("Watch on")
            color: details.theme.subtext
            font.pixelSize: details.unit * 0.022
        }
        Row {
            spacing: details.unit * 0.02

            PillButton {
                id: jellyfinButton
                theme: details.theme
                text: details.item.progress > 0 ? qsTr("Resume in Jellyfin") : qsTr("Jellyfin")
                symbol: "▶"
                focus: true
                KeyNavigation.right: mpvButton.visible ? mpvButton : backButton
                onClicked: {
                    details.closed()
                    TV.watchInJellyfin(details.item.mediaId)
                }
            }
            PillButton {
                id: mpvButton
                visible: TV.hasMpv
                theme: details.theme
                text: qsTr("Play now (mpv)")
                KeyNavigation.left: jellyfinButton
                KeyNavigation.right: backButton
                onClicked: {
                    details.closed()
                    TV.playMedia(details.item.mediaId)
                }
            }
            PillButton {
                id: backButton
                theme: details.theme
                text: qsTr("Back")
                KeyNavigation.left: mpvButton.visible ? mpvButton : jellyfinButton
                onClicked: details.closed()
            }
        }
    }

    Keys.onEscapePressed: details.closed()
    Keys.onBackPressed: details.closed()
}
