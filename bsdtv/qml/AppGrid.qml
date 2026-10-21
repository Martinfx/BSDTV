import QtQuick

GridView {
    id: grid

    property real tileWidth: 200
    property real tileHeight: 112
    property real spacing: 24
    property var theme
    property bool reducedMotion: false
    property string runningApp: ""

    readonly property AppTile currentTile: grid.currentItem as AppTile

    signal appFocused(string name, string caption, string color)
    signal launched(string appId)

    function announce() {
        if (grid.activeFocus && grid.currentTile)
            grid.appFocused(grid.currentTile.name, grid.currentTile.caption, grid.currentTile.color)
    }

    cellWidth: grid.tileWidth + grid.spacing
    cellHeight: grid.tileHeight + grid.spacing * 1.5
    interactive: false
    keyNavigationEnabled: true
    keyNavigationWraps: false
    highlightMoveDuration: grid.reducedMotion ? 0 : 200
    preferredHighlightBegin: grid.spacing
    preferredHighlightEnd: grid.height - grid.cellHeight
    highlightRangeMode: GridView.ApplyRange
    activeFocusOnTab: true

    delegate: AppTile {
        required property int index
        width: grid.tileWidth
        height: grid.tileHeight
        theme: grid.theme
        reducedMotion: grid.reducedMotion
        highlighted: grid.activeFocus && GridView.isCurrentItem
        running: grid.runningApp === appId
    }

    onCurrentTileChanged: grid.announce()
    onActiveFocusChanged: grid.announce()

    Keys.onReturnPressed: if (grid.currentTile) grid.launched(grid.currentTile.appId)
    Keys.onEnterPressed: if (grid.currentTile) grid.launched(grid.currentTile.appId)
}
