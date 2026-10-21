import QtQuick

// Letter grid for typing with the arrow keys of a remote.
GridView {
    id: keyboard

    property var theme
    property real keySize: 48
    readonly property var keys: "abcdefghijklmnopqrstuvwxyz1234567890".split("").concat(["space", "backspace", "clear"])

    signal typed(string text)
    signal erased()
    signal cleared()

    function press(key) {
        if (key === "space")
            keyboard.typed(" ")
        else if (key === "backspace")
            keyboard.erased()
        else if (key === "clear")
            keyboard.cleared()
        else
            keyboard.typed(key)
    }

    width: keyboard.cellWidth * 10
    height: keyboard.cellHeight * Math.ceil(keyboard.keys.length / 10)
    cellWidth: keyboard.keySize * 1.12
    cellHeight: keyboard.keySize * 1.12
    interactive: false
    keyNavigationEnabled: true
    model: keyboard.keys
    activeFocusOnTab: true

    delegate: Rectangle {
        id: key

        required property string modelData
        required property int index
        readonly property bool current: GridView.isCurrentItem && keyboard.activeFocus
        readonly property string name: key.modelData

        width: keyboard.keySize
        height: keyboard.keySize
        radius: keyboard.keySize * 0.2
        color: key.current ? keyboard.theme.focus : Qt.rgba(1, 1, 1, 0.08)

        Accessible.role: Accessible.Button
        Accessible.name: key.modelData

        Text {
            anchors.centerIn: parent
            text: key.modelData === "space" ? "␣" : key.modelData === "backspace" ? "⌫"
                : key.modelData === "clear" ? "✕" : key.modelData.toUpperCase()
            color: key.current ? keyboard.theme.background : keyboard.theme.text
            font.pixelSize: keyboard.keySize * 0.42
            font.weight: Font.DemiBold
        }
    }

    Keys.onReturnPressed: keyboard.press(keyboard.keys[keyboard.currentIndex])
    Keys.onEnterPressed: keyboard.press(keyboard.keys[keyboard.currentIndex])
}
