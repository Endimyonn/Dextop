import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Controls 6.5

Rectangle {
    id: resultBox
    width: 500
    height: 150
    color: "#515151"

    Image {
        id: image
        width: 100
        height: 100
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 20
        source: "qrc:/qtquickplugin/images/template_image.png"
        fillMode: Image.PreserveAspectFit
    }

    Label {
        id: mangaTitle
        width: resultBox.width - image.width - 40
        color: "#ffffff"
        text: qsTr("Label")
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: image.width + 40
        anchors.topMargin: 20
        font.pointSize: 12
    }

    Label {
        id: mangaTitle1
        width: resultBox.width - image.width - 40
        height: 100
        color: "#ffffff"
        text: qsTr("Label")
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: image.width + 40
        anchors.topMargin: 40
        font.pointSize: 12
    }

}
