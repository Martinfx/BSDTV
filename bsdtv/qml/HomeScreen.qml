import QtQuick
import BSDTV

FocusScope {
    id: home

    readonly property var theme: TV.theme
    readonly property bool reducedMotion: TV.settings.reduced_motion === true
    readonly property int columns: Math.max(3, Math.min(7, TV.settings.columns || 5))
    readonly property real margin: home.width * 0.06
    readonly property real spacing: home.width * 0.018
    readonly property real tileWidth: (home.width - 2 * home.margin - (home.columns - 1) * home.spacing) / home.columns
    readonly property real tileHeight: home.tileWidth * 0.5625

    property string focusedName: ""
    readonly property bool browsing: grid.activeFocus
    property string focusedCaption: ""
    property color focusedColor: home.theme.accent

    signal settingsRequested()
    signal powerRequested()

    function track(name, caption, color) {
        home.focusedName = name
        home.focusedCaption = caption
        home.focusedColor = color
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: home.theme.background2 }
            GradientStop { position: 1.0; color: home.theme.background }
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

    AmbientGlow {
        anchors.fill: parent
        glowColor: home.focusedColor
        reducedMotion: home.reducedMotion
    }

    TopBar {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: home.margin * 0.6
        anchors.leftMargin: home.margin
        anchors.rightMargin: home.margin
        unit: home.height
        opacity: home.browsing ? 0 : 1

        Behavior on opacity {
            enabled: !home.reducedMotion
            OpacityAnimator { duration: 200 }
        }

        KeyNavigation.down: dock.count > 0 ? dock : grid
        onSettingsRequested: home.settingsRequested()
        onPowerRequested: home.powerRequested()
    }

    Column {
        id: shelf
        anchors.left: parent.left
        anchors.leftMargin: home.margin
        anchors.bottom: dock.top
        anchors.bottomMargin: home.spacing * 1.5
        spacing: 6

        Text {
            text: home.focusedName
            color: home.theme.text
            font.pixelSize: home.height * 0.06
            font.weight: Font.DemiBold
        }
        Text {
            text: home.focusedCaption
            color: home.theme.subtext
            font.pixelSize: home.height * 0.026
        }
    }

    AppRow {
        id: dock
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: home.margin
        anchors.rightMargin: home.margin
        y: home.browsing ? home.margin * 0.6 + shelf.height + home.spacing : home.height * 0.52
        height: home.tileHeight * 1.25
        model: TV.dockModel
        tileWidth: home.tileWidth
        tileHeight: home.tileHeight
        spacing: home.spacing
        theme: home.theme
        reducedMotion: home.reducedMotion
        runningApp: TV.runningApp
        KeyNavigation.priority: KeyNavigation.AfterItem
        KeyNavigation.up: topBar
        KeyNavigation.down: grid.count > 0 ? grid : null
        onAppFocused: (name, caption, color) => home.track(name, caption, color)
        onLaunched: appId => TV.launch(appId)

        Behavior on y {
            enabled: !home.reducedMotion
            NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
        }
    }

    AppGrid {
        id: grid
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: home.margin
        anchors.rightMargin: home.margin - home.spacing
        anchors.top: dock.bottom
        anchors.bottom: parent.bottom
        model: TV.appModel
        tileWidth: home.tileWidth
        tileHeight: home.tileHeight
        spacing: home.spacing
        theme: home.theme
        reducedMotion: home.reducedMotion
        runningApp: TV.runningApp
        KeyNavigation.priority: KeyNavigation.AfterItem
        KeyNavigation.up: dock.count > 0 ? dock : topBar
        Keys.onEscapePressed: (dock.count > 0 ? dock : topBar).forceActiveFocus()
        onAppFocused: (name, caption, color) => home.track(name, caption, color)
        onLaunched: appId => TV.launch(appId)
    }

    Text {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: home.margin * 0.4
        visible: text !== ""
        text: TV.stats
        color: home.theme.subtext
        font.pixelSize: home.height * 0.018
    }

    Component.onCompleted: (dock.count > 0 ? dock : grid).forceActiveFocus()

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Menu) {
            home.settingsRequested()
            event.accepted = true
        }
    }
}
