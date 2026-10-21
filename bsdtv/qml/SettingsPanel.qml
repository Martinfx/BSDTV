import QtQuick
import BSDTV

FocusScope {
    id: panel

    readonly property var theme: TV.theme
    readonly property var settings: TV.settings
    readonly property var themeIds: TV.themes.map(t => t.id)
    readonly property int themeIndex: Math.max(0, panel.themeIds.indexOf(panel.settings.theme))

    signal closed()

    function cycleTheme(step) {
        const ids = panel.themeIds
        if (ids.length > 0)
            TV.setSetting("theme", ids[(panel.themeIndex + step + ids.length) % ids.length])
    }

    readonly property var languages: ["auto", "cs", "en"]

    function languageName(code) {
        switch (code) {
        case "cs": return "Čeština"
        case "en": return "English"
        }
        return qsTr("Automatic")
    }

    function cycleLanguage(step) {
        const i = Math.max(0, panel.languages.indexOf(panel.settings.language || "auto"))
        TV.setSetting("language", panel.languages[(i + step + panel.languages.length) % panel.languages.length])
    }

    function onOff(value) {
        return value ? qsTr("On") : qsTr("Off")
    }

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.55)
        Accessible.ignored: true
    }

    Rectangle {
        id: sheet
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 32
        width: Math.max(520, parent.width * 0.36)
        radius: 28
        color: panel.theme.surface

        Column {
            anchors.fill: parent
            anchors.margins: 28
            spacing: 6

            Text {
                text: qsTr("Settings")
                color: panel.theme.text
                font.pixelSize: 34
                font.weight: Font.DemiBold
                bottomPadding: 16
            }

            SettingRow {
                id: themeRow
                theme: panel.theme
                label: qsTr("Theme")
                value: TV.themes.length > 0 ? TV.themes[panel.themeIndex].name : ""
                focus: true
                KeyNavigation.down: layoutRow
                onDecrease: panel.cycleTheme(-1)
                onIncrease: panel.cycleTheme(1)
            }
            SettingRow {
                id: layoutRow
                theme: panel.theme
                label: qsTr("Home screen")
                value: panel.settings.layout === "classic" ? qsTr("Classic") : qsTr("TV with rows")
                KeyNavigation.up: themeRow
                KeyNavigation.down: languageRow
                onDecrease: TV.setSetting("layout", panel.settings.layout === "classic" ? "tv" : "classic")
                onIncrease: TV.setSetting("layout", panel.settings.layout === "classic" ? "tv" : "classic")
            }
            SettingRow {
                id: languageRow
                theme: panel.theme
                label: qsTr("Language")
                value: panel.languageName(panel.settings.language || "auto")
                KeyNavigation.up: layoutRow
                KeyNavigation.down: columnsRow
                onDecrease: panel.cycleLanguage(-1)
                onIncrease: panel.cycleLanguage(1)
            }
            SettingRow {
                id: columnsRow
                theme: panel.theme
                label: qsTr("Apps per row")
                value: String(panel.settings.columns)
                KeyNavigation.up: languageRow
                KeyNavigation.down: motionRow
                onDecrease: TV.setSetting("columns", Math.max(3, panel.settings.columns - 1))
                onIncrease: TV.setSetting("columns", Math.min(7, panel.settings.columns + 1))
            }
            SettingRow {
                id: motionRow
                theme: panel.theme
                label: qsTr("Reduce motion")
                value: panel.onOff(panel.settings.reduced_motion)
                KeyNavigation.up: columnsRow
                KeyNavigation.down: clockRow
                onDecrease: TV.setSetting("reduced_motion", !panel.settings.reduced_motion)
                onIncrease: TV.setSetting("reduced_motion", !panel.settings.reduced_motion)
            }
            SettingRow {
                id: clockRow
                theme: panel.theme
                label: qsTr("24-hour clock")
                value: panel.onOff(panel.settings.clock_24h !== false)
                KeyNavigation.up: motionRow
                KeyNavigation.down: statsRow
                onDecrease: TV.setSetting("clock_24h", panel.settings.clock_24h === false)
                onIncrease: TV.setSetting("clock_24h", panel.settings.clock_24h === false)
            }
            SettingRow {
                id: statsRow
                theme: panel.theme
                label: qsTr("System stats")
                value: panel.onOff(panel.settings.show_stats)
                KeyNavigation.up: clockRow
                onDecrease: TV.setSetting("show_stats", !panel.settings.show_stats)
                onIncrease: TV.setSetting("show_stats", !panel.settings.show_stats)
            }

            Item { width: 1; height: 24 }

            Text {
                text: qsTr("Phone remote")
                color: panel.theme.text
                font.pixelSize: 22
                font.weight: Font.DemiBold
            }
            Text {
                width: parent.width
                wrapMode: Text.WrapAnywhere
                text: TV.remoteUrl !== "" ? TV.remoteUrl : qsTr("Not available (no network or disabled)")
                color: panel.theme.accent
                font.pixelSize: 20
            }
            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: qsTr("Open this address on a phone in the same network.")
                color: panel.theme.subtext
                font.pixelSize: 18
            }

            Item { width: 1; height: 24 }

            Text {
                text: qsTr("BSDTV %1 · FreeBSD").arg(TV.version)
                color: panel.theme.subtext
                font.pixelSize: 18
            }
        }
    }

    Keys.onEscapePressed: panel.closed()
    Keys.onBackPressed: panel.closed()
    Keys.onMenuPressed: panel.closed()
}
