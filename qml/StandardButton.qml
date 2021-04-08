import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import MeuiKit 1.0 as Meui

Item {
    id: control

    signal clicked(string text)

    property string text
    property var icon
    property alias textColor: _label.color
    property bool flat: false

    Layout.fillWidth: true
    Layout.fillHeight: true

    property color backgroundColor: Meui.Theme.backgroundColor
    property color hoveredColor: Meui.Theme.darkMode ? Qt.lighter(Meui.Theme.backgroundColor, 1.1)
                                                     : Qt.darker(Meui.Theme.backgroundColor, 1.1)
    property color pressedColor: Meui.Theme.darkMode ? Qt.darker(Meui.Theme.backgroundColor, 1.05)
                                                     : Qt.darker(Meui.Theme.backgroundColor, 1.3)

    property color flatBg: Meui.Theme.highlightColor
    property color flatHoveredBg: Meui.Theme.darkMode ? Qt.lighter(Meui.Theme.highlightColor, 1.1)
                                                      : Qt.darker(Meui.Theme.highlightColor, 1.1)
    property color flatPressedBg: Meui.Theme.darkMode ? Qt.lighter(Meui.Theme.highlightColor, 1.05)
                                                      : Qt.darker(Meui.Theme.highlightColor, 1.3)

    Rectangle {
        id: _background
        anchors.fill: parent
        color: !flat ? _mouseArea.pressed ? pressedColor : _mouseArea.containsMouse
                                            ? hoveredColor : backgroundColor
                     : _mouseArea.pressed ? flatPressedBg : _mouseArea.containsMouse
                                                           ? flatHoveredBg : flatBg
        opacity: !flat ? _mouseArea.pressed || _mouseArea.containsMouse ? 0.7 : 0.5 : 1.0
    }

    MouseArea {
        id: _mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: control.clicked(control.text)
    }

    Image {
        id: _icon
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        width: control.height * 0.4
        height: width
        sourceSize: Qt.size(width, height)
        source: control.icon ? control.icon : ""
        visible: _icon.status === Image.Ready
    }

    Label {
        id: _label
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        Layout.minimumWidth: parent.width
        horizontalAlignment: Text.AlignHCenter
        color: !flat ? Meui.Theme.textColor : Meui.Theme.highlightedTextColor
        text: control.text
        visible: !_icon.visible
        minimumPointSize: Math.round(control.height / 5)
        font.pointSize: Math.round(control.height / 5)
    }
}
