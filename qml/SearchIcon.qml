import QtQuick

// A magnifying glass drawn with rectangles, so it needs no icon font.
Item {
    id: icon

    property color tint: "white"

    Rectangle {
        width: icon.width * 0.62
        height: width
        radius: width / 2
        color: "transparent"
        border.width: Math.max(2, icon.width * 0.1)
        border.color: icon.tint
    }
    Rectangle {
        x: icon.width * 0.52
        y: icon.height * 0.66
        width: icon.width * 0.42
        height: Math.max(2, icon.width * 0.11)
        radius: height / 2
        rotation: 45
        transformOrigin: Item.Left
        color: icon.tint
    }
}
