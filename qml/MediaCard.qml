import QtQuick

// A film or episode: a wide still with rounded corners and a progress bar.
Item {
    id: card

    required property var item
    property var theme
    property bool highlighted: false
    property bool reducedMotion: false
    readonly property real cornerRadius: card.height * 0.06

    Accessible.role: Accessible.Button
    Accessible.name: card.item.title

    Item {
        id: body
        anchors.fill: parent
        scale: card.highlighted ? 1.1 : 1.0

        Behavior on scale {
            enabled: !card.reducedMotion
            NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
        }

        Rectangle {
            id: frame
            anchors.fill: parent
            radius: card.cornerRadius
            color: card.theme.surface
            clip: true

            Image {
                id: still
                anchors.fill: parent
                source: card.item.image
                sourceSize.width: width
                sourceSize.height: height
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
            }

            // Title over a plain card while the image loads or when there is none.
            Text {
                anchors.fill: parent
                anchors.margins: parent.height * 0.1
                visible: still.status !== Image.Ready
                text: card.item.title
                color: card.theme.text
                wrapMode: Text.WordWrap
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: parent.height * 0.14
                font.weight: Font.DemiBold
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: parent.height * 0.045
                visible: card.item.progress > 0
                color: Qt.rgba(0, 0, 0, 0.5)

                Rectangle {
                    width: parent.width * card.item.progress
                    height: parent.height
                    color: card.theme.accent
                }
            }
        }

        // Rounds the picture's corners by painting the background over them. Works with
        // every scene graph backend, unlike shader masks (no GPU needed).
        Rectangle {
            anchors.fill: parent
            anchors.margins: -card.cornerRadius
            radius: card.cornerRadius * 2
            color: "transparent"
            border.width: card.cornerRadius
            border.color: card.theme.background
        }

        Rectangle {
            anchors.fill: parent
            radius: card.cornerRadius
            color: "transparent"
            border.width: card.highlighted ? 3 : 1
            border.color: card.highlighted ? card.theme.focus : Qt.rgba(1, 1, 1, 0.08)
        }
    }
}
