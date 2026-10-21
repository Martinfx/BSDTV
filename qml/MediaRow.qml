import QtQuick
import BSDTV

// A row of films and episodes from Jellyfin ("Continue watching", "Recently added", search results).
FocusScope {
    id: mediaRow

    property string title: ""
    property var items: []
    property real unit: 720
    property real cardWidth: 280
    property real inset: 0
    property bool reducedMotion: false
    readonly property real cardHeight: mediaRow.cardWidth * 0.5625

    signal activated(var item)

    height: heading.height + list.height
    activeFocusOnTab: true

    Text {
        id: heading
        x: mediaRow.inset
        text: mediaRow.title
        color: mediaRow.activeFocus ? TV.theme.text : TV.theme.subtext
        font.pixelSize: mediaRow.unit * 0.026
        font.weight: Font.DemiBold
        bottomPadding: mediaRow.unit * 0.014
    }

    ListView {
        id: list
        anchors.top: heading.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: mediaRow.cardHeight + mediaRow.unit * 0.075
        orientation: ListView.Horizontal
        spacing: mediaRow.unit * 0.024
        model: mediaRow.items
        focus: true
        interactive: false
        keyNavigationEnabled: true
        highlightMoveDuration: mediaRow.reducedMotion ? 0 : 200
        preferredHighlightBegin: mediaRow.inset
        preferredHighlightEnd: list.width - mediaRow.cardWidth * 1.5
        highlightRangeMode: ListView.ApplyRange
        leftMargin: mediaRow.inset + mediaRow.unit * 0.012

        delegate: Item {
            id: cell

            required property var modelData
            required property int index
            readonly property bool current: ListView.isCurrentItem && list.activeFocus
            // Exposed for tests and accessibility tools.
            readonly property string name: cell.modelData.title

            width: mediaRow.cardWidth
            height: list.height

            MediaCard {
                width: mediaRow.cardWidth
                height: mediaRow.cardHeight
                y: mediaRow.unit * 0.008
                item: cell.modelData
                theme: TV.theme
                highlighted: cell.current
                reducedMotion: mediaRow.reducedMotion
            }

            Column {
                anchors.bottom: parent.bottom
                width: parent.width
                visible: cell.current

                Text {
                    width: parent.width
                    text: cell.modelData.title
                    color: TV.theme.text
                    elide: Text.ElideRight
                    font.pixelSize: mediaRow.unit * 0.021
                    font.weight: Font.DemiBold
                }
                Text {
                    width: parent.width
                    text: cell.modelData.subtitle
                    color: TV.theme.subtext
                    elide: Text.ElideRight
                    font.pixelSize: mediaRow.unit * 0.018
                }
            }
        }

        Keys.onReturnPressed: if (list.currentIndex >= 0) mediaRow.activated(mediaRow.items[list.currentIndex])
        Keys.onEnterPressed: if (list.currentIndex >= 0) mediaRow.activated(mediaRow.items[list.currentIndex])
    }
}
