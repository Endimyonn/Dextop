import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Controls 6.5
import Dextop 1.0

Window {
    id: window
    visible: true
    width: 640
    height: 500
    title: "Dextop"

    UIProxy {
        id: uiProxy
    }

    Rectangle {
        id: backgroundFill
        width: window.width
        height: window.height
        color: "#1c1c1c"

        Column {
            id: navBar
            width: 50
            height: backgroundFill.height

            Rectangle {
                id: area1
                width: navBar.width
                height: 50
                color: "#5d5d5d"

                Button {
                    id: navButton1
                    width: area1.width
                    height: area1.height
                    text: qsTr("Home")
                    onClicked: {
                        homePane.visible = true;
                        testPane.visible = false;
                    }
                }
            }

            Rectangle {
                id: area2
                width: navBar.width
                height: 50
                color: "#5d5d5d"

                Button {
                    id: navButton2
                    width: area2.width
                    height: area2.height
                    text: qsTr("Test")
                    onClicked: {
                        homePane.visible = false;
                        testPane.visible = true;
                    }
                }
            }
        }

        Rectangle {
            id: homePane
            color: "#3c3c3c"
            width: backgroundFill.width - navBar.width
            height: window.height
            anchors.right: parent.right
            anchors.rightMargin: 0

            Text {
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                anchors.horizontalCenter: parent.horizontalCenter
                font.bold: true
                font.pointSize: 42
                color: "#ffffff"
                text: "Hello World!"
                anchors.top: parent.top
                anchors.topMargin: 12
            }

            Button {
                id: button
                width: 100
                height: 40
                text: qsTr("Button")
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        Rectangle {
            id: testPane
            color: "#3c3c3c"
            width: backgroundFill.width - navBar.width
            height: window.height
            anchors.right: parent.right
            anchors.rightMargin: 0
            Text {
                text: "wowow"
                anchors.top: parent.top
                anchors.topMargin: 12
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pointSize: 42
                font.bold: true
                color: "#ffffff"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Button {
                id: button1
                width: 100
                height: 40
                text: qsTr("Button")
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: 139
                anchors.horizontalCenterOffset: 0
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: uiProxy.testFunction1()
            }
        }
    }
}
