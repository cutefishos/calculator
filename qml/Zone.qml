import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.5
import QtQuick.Controls.Styles 1.4

import FishUI 1.0 as FishUI
import Cutefish.Calculator 1.0

Item {
    id: zone

    ColumnLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: 0

        ListView {
            id: listView
            model: ListModel { id: historyModel }
            clip: true

            Layout.fillHeight: true
            Layout.fillWidth: true

            flickableDirection: Flickable.VerticalFlick
            ScrollBar.vertical: ScrollBar {}

            onCountChanged: {
                listView.currentIndex = count - 1
            }

            delegate: Item {
                height: label.implicitHeight + FishUI.Units.largeSpacing * 2
                width: parent ? parent.width : undefined

                Label {
                    id: label
                    anchors.fill: parent
                    horizontalAlignment: Qt.AlignRight
                    text: historyModel.get(index).text
                    elide: Text.ElideMiddle
                    color: FishUI.Theme.disabledTextColor

                    leftPadding: FishUI.Units.largeSpacing
                    rightPadding: FishUI.Units.largeSpacing

                    MouseArea {
                        hoverEnabled: true
                    }
                }
            }
        }

        TextField {
            id: textField
            Layout.preferredHeight: 50
            Layout.fillWidth: true
            Keys.onReturnPressed: appendToTextField('=')
            Keys.onEnterPressed: appendToTextField('=')

            selectByMouse: true
            horizontalAlignment: TextInput.AlignRight
            focus: Qt.StrongFocus
            font.pixelSize: 24

            background: Rectangle {
                border.width: 0
                color: "transparent"
            }

            leftPadding: FishUI.Units.largeSpacing
            rightPadding: FishUI.Units.largeSpacing
        }
    }

    function appendToTextField(text) {
        if (text === '=') {
            var res = calculate(textField.text)
            if (res !== '') {
                var expressionText = textField.text
                textField.text = res

                // If the expression and the result area equal,
                // it will not be added to the ListView
                if (expressionText !== res) {
                    expressionText = expressionText + " = " + res
                    historyModel.append({"text": expressionText})
                }
            }
        } else if (text === 'C') {
            if (textField.text != "")
                textField.clear()
            else 
                historyModel.clear()
        } else if (text === 'BACKSPACE') {
            // backspace
            textField.remove(textField.cursorPosition, textField.cursorPosition - 1)
        } else {
            textField.insert(textField.cursorPosition, text)
        }
    }
}
