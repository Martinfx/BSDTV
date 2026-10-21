import QtQuick
import BSDTV

// Search across apps and the Jellyfin library. Type with the on-screen keyboard,
// a keyboard, or the phone remote ("Type on the TV").
FocusScope {
    id: panel

    readonly property var theme: TV.theme
    readonly property real unit: panel.height
    readonly property real margin: panel.width * 0.055
    readonly property var results: TV.searchResults
    property string query: ""

    signal closed()

    function setQuery(text) {
        panel.query = text
        debounce.restart()
    }

    Timer {
        id: debounce
        interval: 350
        onTriggered: TV.search(panel.query)
    }

    Component.onCompleted: TV.search("")

    Rectangle {
        anchors.fill: parent
        color: panel.theme.background
    }

    Rectangle {
        id: field
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: panel.unit * 0.06
        anchors.leftMargin: panel.margin
        anchors.rightMargin: panel.margin
        height: panel.unit * 0.08
        radius: height / 2
        color: panel.theme.surface

        SearchIcon {
            id: icon
            anchors.left: parent.left
            anchors.leftMargin: field.height * 0.4
            anchors.verticalCenter: parent.verticalCenter
            width: field.height * 0.4
            height: width
            tint: panel.theme.subtext
        }
        Text {
            anchors.left: icon.right
            anchors.right: parent.right
            anchors.leftMargin: field.height * 0.3
            anchors.verticalCenter: parent.verticalCenter
            text: panel.query !== "" ? panel.query + "▏" : qsTr("Search apps, films and series")
            color: panel.query !== "" ? panel.theme.text : panel.theme.subtext
            elide: Text.ElideLeft
            font.pixelSize: panel.unit * 0.032
        }
    }

    OnScreenKeyboard {
        id: keyboard
        anchors.top: field.bottom
        anchors.left: parent.left
        anchors.topMargin: panel.unit * 0.05
        anchors.leftMargin: panel.margin
        keySize: panel.unit * 0.068
        theme: panel.theme
        focus: true
        KeyNavigation.priority: KeyNavigation.AfterItem
        KeyNavigation.right: appResults.visible ? appResults : (mediaResults.visible ? mediaResults : null)
        onTyped: text => panel.setQuery(panel.query + text)
        onErased: panel.setQuery(panel.query.slice(0, -1))
        onCleared: panel.setQuery("")
    }

    Column {
        id: resultsColumn
        anchors.top: keyboard.top
        anchors.left: keyboard.right
        anchors.right: parent.right
        anchors.leftMargin: panel.unit * 0.05
        spacing: panel.unit * 0.04

        ContentRow {
            id: appResults
            width: parent.width
            visible: panel.results.apps.length > 0
            title: qsTr("Apps")
            apps: panel.results.apps
            round: true
            unit: panel.unit
            tileWidth: panel.unit * 0.3
            reducedMotion: TV.settings.reduced_motion === true
            runningApp: TV.runningApp
            KeyNavigation.priority: KeyNavigation.AfterItem
            KeyNavigation.left: keyboard
            KeyNavigation.down: mediaResults.visible ? mediaResults : null
            onLaunched: appId => {
                panel.closed()
                TV.launch(appId)
            }
        }

        MediaRow {
            id: mediaResults
            width: parent.width
            visible: panel.results.media.length > 0
            title: qsTr("Films and series")
            items: panel.results.media
            unit: panel.unit
            cardWidth: panel.unit * 0.36
            reducedMotion: TV.settings.reduced_motion === true
            KeyNavigation.priority: KeyNavigation.AfterItem
            KeyNavigation.left: keyboard
            KeyNavigation.up: appResults.visible ? appResults : null
            onActivated: item => TV.openMedia(item.mediaId)
        }

        Text {
            visible: panel.results.busy || (panel.results.term !== "" && panel.results.apps.length === 0 && panel.results.media.length === 0)
            text: panel.results.busy ? qsTr("Searching…") : qsTr("Nothing found")
            color: panel.theme.subtext
            font.pixelSize: panel.unit * 0.028
        }
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape || event.key === Qt.Key_Back) {
            panel.closed()
            event.accepted = true
        } else if (event.key === Qt.Key_Backspace) {
            panel.setQuery(panel.query.slice(0, -1))
            event.accepted = true
        } else if (event.text.length > 0 && event.text.charCodeAt(0) >= 32) {
            panel.setQuery(panel.query + event.text)
            event.accepted = true
        }
    }
}
