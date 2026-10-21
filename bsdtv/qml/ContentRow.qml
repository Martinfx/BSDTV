import QtQuick
import BSDTV

FocusScope {
    id: contentRow

    property string title: ""
    property var apps: []
    property real unit: 720
    property real tileWidth: 240
    property bool reducedMotion: false
    property string runningApp: ""
    readonly property real tileHeight: contentRow.tileWidth * 0.5625

    signal launched(string appId)

    height: heading.height + list.height
    activeFocusOnTab: true

    Text {
        id: heading
        text: contentRow.title
        color: contentRow.activeFocus ? TV.theme.text : TV.theme.subtext
        font.pixelSize: contentRow.unit * 0.026
        font.weight: Font.DemiBold
        bottomPadding: contentRow.unit * 0.014
    }

    ListView {
        id: list
        anchors.top: heading.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: contentRow.tileHeight * 1.32
        orientation: ListView.Horizontal
        spacing: contentRow.unit * 0.024
        model: contentRow.apps
        focus: true
        interactive: false
        keyNavigationEnabled: true
        highlightMoveDuration: contentRow.reducedMotion ? 0 : 200
        preferredHighlightBegin: 0
        preferredHighlightEnd: list.width - contentRow.tileWidth * 1.2
        highlightRangeMode: ListView.ApplyRange
        // Room for the focus zoom on the first tile.
        leftMargin: contentRow.unit * 0.012
        clip: false

        delegate: Item {
            id: cell

            required property var modelData
            required property int index
            readonly property bool current: ListView.isCurrentItem && list.activeFocus

            width: contentRow.tileWidth
            height: list.height

            AppTile {
                width: contentRow.tileWidth
                height: contentRow.tileHeight
                y: contentRow.unit * 0.008
                appId: cell.modelData.appId
                name: cell.modelData.name
                caption: cell.modelData.caption
                color: cell.modelData.color
                icon: cell.modelData.icon
                theme: TV.theme
                reducedMotion: contentRow.reducedMotion
                highlighted: cell.current
                running: contentRow.runningApp === cell.modelData.appId
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                visible: cell.current
                text: cell.modelData.name
                color: TV.theme.text
                font.pixelSize: contentRow.unit * 0.022
            }
        }

        Keys.onReturnPressed: if (list.currentIndex >= 0) contentRow.launched(contentRow.apps[list.currentIndex].appId)
        Keys.onEnterPressed: if (list.currentIndex >= 0) contentRow.launched(contentRow.apps[list.currentIndex].appId)
    }
}
