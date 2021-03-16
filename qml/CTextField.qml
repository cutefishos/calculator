import QtQuick 2.0
import QtQuick.Controls 2.5

TextField {
    id: textField
    selectByMouse: true
    horizontalAlignment: TextInput.AlignRight
    focus: Qt.StrongFocus
    font.pixelSize: 24

    property int selectStart
    property int selectEnd
    property int curPos

    background: Rectangle {
        border.width: 0
        color: "transparent"
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.IBeamCursor
        acceptedButtons: Qt.RightButton

        onClicked: {
            selectStart = textField.selectionStart;
            selectEnd = textField.selectionEnd;
            curPos = textField.cursorPosition;
            contextMenu.x = mouse.x;
            contextMenu.y = mouse.y;
            contextMenu.open();
            textField.cursorPosition = curPos;
            textField.select(selectStart, selectEnd);
        }
    }

    Menu {
        id: contextMenu
        MenuItem {
            text: qsTr("Cut")
            onTriggered: {
                textField.cut()
            }
        }
        MenuItem {
            text: qsTr("Copy")
            onTriggered: {
                textField.copy()
            }
        }
        MenuItem {
            text: qsTr("Paste")
            onTriggered: {
                textField.paste()
            }
        }
    }
}
