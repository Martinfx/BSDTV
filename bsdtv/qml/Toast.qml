import QtQuick

Rectangle {
    id: toast

    property var theme

    function show(text) {
        label.text = text
        toast.visible = true
        hideTimer.restart()
    }

    visible: false
    width: label.implicitWidth + 48
    height: label.implicitHeight + 24
    radius: height / 2
    color: toast.theme.surface

    Text {
        id: label
        anchors.centerIn: parent
        color: toast.theme.text
        font.pixelSize: 20
    }

    Timer {
        id: hideTimer
        interval: 4000
        onTriggered: toast.visible = false
    }
}
