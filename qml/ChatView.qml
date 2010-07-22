import QtQuick 1.1
// import Qt.components 1.0


ListView {
  id: flickable
  anchors.fill: parent

  Component {
      id: msgDelegate
      Item {
        Row {
          id: chatline
          Text { text: timestamp; wrapMode: Text.NoWrap; width: 100 }
          Text { text: sender; wrapMode: Text.NoWrap; width: 100 }
          Text { text: contents; wrapMode: Text.Wrap; width: flickable.width-200}
        }
        height: chatline.height
        ListView.onAdd: positionViewAtEnd()
      }
  }

  model: msgModel

  delegate: msgDelegate

  Rectangle {
    id: scrollbar
    anchors.right: flickable.right
    y: flickable.visibleArea.yPosition * flickable.height
           width: 10
           height: flickable.visibleArea.heightRatio * flickable.height
           color: "black"
  }
}
