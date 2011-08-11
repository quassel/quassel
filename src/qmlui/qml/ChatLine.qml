import QtQuick 1.1

Item {
  Row {
    id: chatLine
    Text { text: timestamp; wrapMode: Text.NoWrap; width: 100 }
    Text { text: sender; wrapMode: Text.NoWrap; width: 100 }
    Text { text: contents; wrapMode: Text.Wrap; width: chatView.width-200}
  }
  height: chatLine.height
}
