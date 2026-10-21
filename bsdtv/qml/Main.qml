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

    HomeScreen {
        id: home
        objectName: "home"
        anchors.fill: parent
        focus: true
        onSettingsRequested: overlay.show(settingsComponent)
        onPowerRequested: overlay.show(powerComponent)
    }

    Loader {
        id: overlay

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

    Connections {
        target: TV
        function onMessage(text) { toast.show(text) }
        function onRaiseRequested() {
            if (!overlay.active)
                home.forceActiveFocus()
        }
    }
}
