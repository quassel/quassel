import QtQuick 1.1
// import Qt.components 1.0

import eu.quassel.qml 1.0

Rectangle {
    id: container

    ListView {
        id: chatView

        property int timestampWidth: 50
        property int senderWidth: 80
        property int contentsWidth: width - timestampWidth - senderWidth - 30;
        property int columnSpacing: 10

        anchors.fill: parent
        model: msgModel

        delegate: ChatLine {
            id: chatLineDelegate

            timestampWidth: chatView.timestampWidth
            senderWidth: chatView.senderWidth
            contentsWidth: chatView.contentsWidth
            columnSpacing: chatView.columnSpacing
            model: chatView.model
            renderData: renderDataRole

/*
                MouseArea {
                    id: itemMouseArea
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    hoverEnabled: true

                    //onClicked: {
                    //    console.log("clicked " + mouseX + " " + mouseY + " " + parent.text)
                    //    parent.onClicked(mouseX, mouseY)
                    //}

                    //onPositionChanged: {
                    //    console.log("changed " + mouseX + " " + mouseY + " " + parent.text)
                    //}

                }
*/
/*
                Connections {
                    target: itemMouseArea
                    onClicked: onClicked(itemMouseArea.mouseX, itemMouseArea.mouseY)
                    onPressed: onPressed(itemMouseArea.mouseX, itemMouseArea.mouseY)
                    onPositionChanged: onMousePositionChanged(itemMouseArea.mouseX, itemMouseArea.mouseY)
                }
*/
        }

        interactive: true
        boundsBehavior: Flickable.StopAtBounds

        Connections {
            target: msgModel
            onRowsInserted: chatView.positionViewAtEnd();
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            //hoverEnabled: true

            onClicked: {
                //parent.senderWidth = parent.senderWidth + 10
                //chatView.currentItem.senderWidth = 40;
                //var pos = mapToItem(chatView, mouseX, mouseY)
                //console.log(pos.x + " " + pos.y + " " + chatView.contentY)
                console.log("clicked " + mouseX + " " + mouseY + " " + chatView.indexAt(mouseX, mouseY + chatView.contentY))
                console.log("item " + chatView.model.get(17).text)
            }
            onPositionChanged: {
                console.log("changed " + mouseX + " " + mouseY)
            }

        }


        Rectangle {
            id: scrollbar
            anchors.right: chatView.right
            y: chatView.visibleArea.yPosition * chatView.height
            width: 10
            height: chatView.visibleArea.heightRatio * chatView.height
            color: "grey"
        }
    }
}
