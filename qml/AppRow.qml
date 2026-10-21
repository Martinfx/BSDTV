import QtQuick

ListView {
    id: row

    property real tileWidth: 200
    property real tileHeight: 112
    property var theme
    property bool reducedMotion: false
    property string runningApp: ""

    readonly property AppTile currentTile: row.currentItem as AppTile

    signal appFocused(string name, string caption, string color)
    signal launched(string appId)

    function announce() {
        if (row.activeFocus && row.currentTile)
            row.appFocused(row.currentTile.name, row.currentTile.caption, row.currentTile.color)
    }

    orientation: ListView.Horizontal
    interactive: false
    keyNavigationEnabled: true
    keyNavigationWraps: false
    highlightMoveDuration: row.reducedMotion ? 0 : 200
    preferredHighlightBegin: 0
    preferredHighlightEnd: row.width - row.tileWidth
    highlightRangeMode: ListView.ApplyRange
    activeFocusOnTab: true

    delegate: AppTile {
        required property int index
        width: row.tileWidth
        height: row.tileHeight
        y: (row.height - height) / 2
        theme: row.theme
        reducedMotion: row.reducedMotion
        highlighted: row.activeFocus && ListView.isCurrentItem
        running: row.runningApp === appId
    }

    onCurrentTileChanged: row.announce()
    onActiveFocusChanged: row.announce()
    onCountChanged: row.announce()

    Keys.onReturnPressed: if (row.currentTile) row.launched(row.currentTile.appId)
    Keys.onEnterPressed: if (row.currentTile) row.launched(row.currentTile.appId)
}
