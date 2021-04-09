import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import Cutefish.Calculator 1.0
import FishUI 1.0 as FishUI

FishUI.Window {
    visible: true
    width: 350
    height: 550
    minimumWidth: 350
    minimumHeight: 550
    title: qsTr("Calculator")
    id: rootWindow

    backgroundColor: FishUI.Theme.darkMode ? Qt.rgba(46 / 255, 46 / 255, 46 / 255, 1.0)
                                         : Qt.rgba(240 / 255, 238 / 255, 241 / 255, 1.0)

    CalcEngine {
        id: calcEngine

        Component.onCompleted: {
            console.log("load calc engine finished")
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: FishUI.Units.smallSpacing

        Zone {
            id: zone
            Layout.fillWidth: true
            Layout.preferredHeight: parent.height * 0.35
        }

        StandardPad {
            Layout.fillWidth: true
            Layout.fillHeight: true
            onPressed: zone.appendToTextField(text)
        }
    }

    function calculate(evalText) {
        var res = calcEngine.eval(evalText)
        return res
    }
}
