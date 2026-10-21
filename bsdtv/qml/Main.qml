import QtQuick
import QtQuick.Controls
import BSDTV

ApplicationWindow {
    id: root

    required property bool windowed

    width: 1280
    height: 720
    visible: true
    visibility: root.windowed ? Window.Windowed : Window.FullScreen
    title: "BSDTV"
    color: TV.theme.background

    Loader {
        id: home
        objectName: "home"
        anchors.fill: parent
        focus: true
        sourceComponent: TV.settings.layout === "classic" ? classicComponent : tvComponent
    }

    Component {
        id: tvComponent
        TvHome {
            focus: true
            onSettingsRequested: overlay.show(settingsComponent)
            onPowerRequested: overlay.show(powerComponent)
            onSearchRequested: overlay.show(searchComponent)
            onProfileRequested: overlay.show(profileComponent)
        }
    }

    Component {
        id: classicComponent
        HomeScreen {
            focus: true
            onSettingsRequested: overlay.show(settingsComponent)
            onPowerRequested: overlay.show(powerComponent)
        }
    }

    Loader {
        id: overlay
        property var media: ({})

        function show(component) {
            overlay.sourceComponent = component
            overlay.active = true
            overlay.forceActiveFocus()
        }

        function hide() {
            overlay.active = false
            home.forceActiveFocus()
        }

        anchors.fill: parent
        active: false
        focus: active
    }

    Component {
        id: settingsComponent
        SettingsPanel {
            focus: true
            onClosed: overlay.hide()
        }
    }

    Component {
        id: searchComponent
        SearchPanel {
            focus: true
            onClosed: overlay.hide()
        }
    }

    Component {
        id: profileComponent
        ProfilePicker {
            focus: true
            onClosed: overlay.hide()
        }
    }

    Component {
        id: detailsComponent
        MediaDetails {
            item: overlay.media
            focus: true
            onClosed: overlay.hide()
        }
    }

    Component {
        id: powerComponent
        PowerMenu {
            focus: true
            onClosed: overlay.hide()
        }
    }

    Toast {
        id: toast
        theme: TV.theme
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 48
    }

    Loader {
        id: ambient
        anchors.fill: parent
        z: 10
        active: TV.ambientActive
        focus: active
        sourceComponent: Ambient {
            focus: true
        }
        onActiveChanged: if (!active) (overlay.active ? overlay : home).forceActiveFocus()
    }

    Connections {
        target: TV
        function onMessage(text) { toast.show(text) }
        function onMediaRequested(item) {
            overlay.media = item
            // Later: this may come from a panel that the overlay is about to replace.
            Qt.callLater(overlay.show, detailsComponent)
        }
        function onRaiseRequested() {
            if (!overlay.active)
                home.forceActiveFocus()
        }
    }
}
