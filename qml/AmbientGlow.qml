import QtQuick
import QtQuick.Shapes

Shape {
    id: glow

    property color glowColor: "#7aa2ff"
    property bool reducedMotion: false

    Accessible.ignored: true

    ShapePath {
        strokeWidth: -1
        fillGradient: RadialGradient {
            centerX: glow.width * 0.3
            centerY: glow.height * 0.62
            centerRadius: glow.width * 0.55
            focalX: centerX
            focalY: centerY
            GradientStop { position: 0.0; color: Qt.rgba(glow.glowColor.r, glow.glowColor.g, glow.glowColor.b, 0.30) }
            GradientStop { position: 0.5; color: Qt.rgba(glow.glowColor.r, glow.glowColor.g, glow.glowColor.b, 0.10) }
            GradientStop { position: 1.0; color: "transparent" }
        }
        PathRectangle {
            width: glow.width
            height: glow.height
        }
    }

    Behavior on glowColor {
        enabled: !glow.reducedMotion
        ColorAnimation { duration: 400 }
    }
}
