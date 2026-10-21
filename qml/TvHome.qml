import QtQuick
import BSDTV

FocusScope {
    id: home

    readonly property var theme: TV.theme
    readonly property bool reducedMotion: TV.settings.reduced_motion === true
    readonly property real unit: home.height
    readonly property real margin: home.width * 0.055
    readonly property int columns: Math.max(4, Math.min(8, (TV.settings.columns || 5) + 1))
    // About seven and a half cards across, the last one cut by the screen edge.
    readonly property real tileWidth: (home.width - home.margin) / (home.columns + 1.6)
    readonly property real gridSpacing: home.unit * 0.024
    readonly property real gridTileWidth: (home.width - 2 * home.margin - (home.columns - 1) * home.gridSpacing) / home.columns
    readonly property Item page: topBar.currentTab === 0 ? forYou : appsGrid

    signal settingsRequested()
    signal powerRequested()
    signal searchRequested()
    signal profileRequested()

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: home.theme.background2 }
            GradientStop { position: 0.6; color: home.theme.background }
        }
    }

    Image {
        anchors.fill: parent
        visible: home.theme.wallpaper !== ""
        source: home.theme.wallpaper ? "file://" + home.theme.wallpaper : ""
        sourceSize.width: home.width
        sourceSize.height: home.height
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        Accessible.ignored: true
    }

    TvTopBar {
        id: topBar
        z: 1
        // Out of the way once you scroll into the rows, back when you return to the top.
        opacity: forYou.currentIndex === 0 || topBar.activeFocus || topBar.currentTab !== 0 ? 1 : 0

        Behavior on opacity {
            enabled: !home.reducedMotion
            OpacityAnimator { duration: 200 }
        }

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: home.unit * 0.045
        anchors.leftMargin: home.margin
        anchors.rightMargin: home.margin
        unit: home.unit
        KeyNavigation.down: home.page
        onSettingsRequested: home.settingsRequested()
        onPowerRequested: home.powerRequested()
        onSearchRequested: home.searchRequested()
        onProfileRequested: home.profileRequested()
    }

    ListView {
        id: forYou
        // Full screen: the banner sits under the tab bar and rows run off the right edge.
        anchors.fill: parent
        visible: topBar.currentTab === 0
        focus: true
        clip: true
        interactive: false
        keyNavigationEnabled: true
        spacing: home.unit * 0.035
        bottomMargin: home.unit * 0.12
        highlightMoveDuration: home.reducedMotion ? 0 : 250
        preferredHighlightBegin: home.unit * 0.08
        preferredHighlightEnd: forYou.height * 0.85
        highlightRangeMode: ListView.ApplyRange
        model: 1 + TV.rows.length
        KeyNavigation.priority: KeyNavigation.AfterItem
        KeyNavigation.up: topBar

        delegate: Loader {
            id: rowLoader

            required property int index
            readonly property var row: rowLoader.index > 0 ? TV.rows[rowLoader.index - 1] : null

            width: forYou.width
            sourceComponent: rowLoader.index === 0 ? heroComponent
                           : rowLoader.row && rowLoader.row.style === "media" ? mediaComponent : rowComponent
        }
    }

    Component {
        id: heroComponent
        HeroBanner {
            height: home.unit * 0.5
            inset: home.margin
            focus: true
            unit: home.unit
            slides: TV.featured
            reducedMotion: home.reducedMotion
            onActivated: slide => slide.mediaId ? TV.openMedia(slide.mediaId) : TV.launch(slide.appId)
        }
    }

    Component {
        id: mediaComponent
        MediaRow {
            // The Loader delegate above carries this row's data.
            readonly property var row: parent ? parent.row : null
            focus: true
            unit: home.unit
            title: home.rowTitle(row ? row.key : "")
            items: row ? row.items : []
            cardWidth: home.tileWidth * 1.3
            inset: home.margin
            reducedMotion: home.reducedMotion
            onActivated: item => TV.openMedia(item.mediaId)
        }
    }

    Component {
        id: rowComponent
        ContentRow {
            // The Loader delegate above carries this row's data.
            readonly property var row: parent ? parent.row : null
            focus: true
            unit: home.unit
            title: home.rowTitle(row ? row.key : "")
            apps: row ? row.apps : []
            round: row ? row.style === "round" : false
            inset: home.margin
            tileWidth: home.tileWidth
            reducedMotion: home.reducedMotion
            runningApp: TV.runningApp
            onLaunched: appId => TV.launch(appId)
        }
    }

    function rowTitle(key) {
        switch (key) {
        case "your_apps": return qsTr("Your apps")
        case "continue": return qsTr("Continue watching")
        case "latest": return qsTr("Recently added")
        case "streaming": return qsTr("Streaming")
        case "media": return qsTr("Movies & music")
        case "games": return qsTr("Games")
        }
        return ""
    }

    AppGrid {
        id: appsGrid
        anchors.top: topBar.bottom
        anchors.topMargin: home.unit * 0.06
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: home.margin
        anchors.rightMargin: home.margin - home.gridSpacing
        visible: topBar.currentTab === 1
        model: TV.allAppsModel
        tileWidth: home.gridTileWidth
        tileHeight: home.gridTileWidth * 0.5625
        spacing: home.gridSpacing
        theme: home.theme
        reducedMotion: home.reducedMotion
        runningApp: TV.runningApp
        KeyNavigation.priority: KeyNavigation.AfterItem
        KeyNavigation.up: topBar
        Keys.onEscapePressed: topBar.forceActiveFocus()
        onLaunched: appId => TV.launch(appId)
    }

    Component.onCompleted: forYou.forceActiveFocus()

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Menu) {
            home.settingsRequested()
            event.accepted = true
        } else if (event.key === Qt.Key_Escape && !topBar.activeFocus) {
            // Back from deep in the page jumps to the top, like on a TV.
            forYou.currentIndex = 0
            topBar.forceActiveFocus()
            event.accepted = true
        }
    }
}
