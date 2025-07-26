/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3.                                           *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "multilineedit.h"

#include <QApplication>
#include <QMessageBox>
#include <QScrollBar>

#include "actioncollection.h"
#include "bufferview.h"
#include "graphicalui.h"
#include "tabcompleter.h"

const int leftMargin = 3;
#define MultiLineEditParent QTextEdit

MultiLineEdit::MultiLineEdit(QWidget* parent)
    : MultiLineEditParent(parent)
{
    document()->setDocumentMargin(0);

    setAcceptRichText(false);
#ifdef HAVE_KDE
    enableFindReplace(false);
#endif

#if defined HAVE_SONNET && !defined HAVE_KDE
    _spellCheckDecorator = new Sonnet::SpellCheckDecorator(this);
    highlighter()->setActive(highlighter()->checkerEnabledByDefault());
#endif

    setMode(SingleLine);
    setLineWrapEnabled(false);
    reset();

    // Prevent QTextHtmlImporter::appendNodeText from eating whitespace
    document()->setDefaultStyleSheet("span { white-space: pre-wrap; }");

    connect(this, &QTextEdit::textChanged, this, &MultiLineEdit::on_textChanged);

    _mircColorMap["00"] = "#ffffff";
    _mircColorMap["01"] = "#000000";
    _mircColorMap["02"] = "#000080";
    _mircColorMap["03"] = "#008000";
    _mircColorMap["04"] = "#ff0000";
    _mircColorMap["05"] = "#800000";
    _mircColorMap["06"] = "#800080";
    _mircColorMap["07"] = "#ffa500";
    _mircColorMap["08"] = "#ffff00";
    _mircColorMap["09"] = "#00ff00";
    _mircColorMap["10"] = "#008080";
    _mircColorMap["11"] = "#00ffff";
    _mircColorMap["12"] = "#4169e1";
    _mircColorMap["13"] = "#ff00ff";
    _mircColorMap["14"] = "#808080";
    _mircColorMap["15"] = "#c0c0c0";
}

MultiLineEdit::~MultiLineEdit()
{
    // Empty destructor, may include cleanup if needed
}

#if defined HAVE_SONNET && !defined HAVE_KDE
Sonnet::Highlighter* MultiLineEdit::highlighter() const
{
    return _spellCheckDecorator->highlighter();
}

void MultiLineEdit::setSpellCheckEnabled(bool enabled)
{
    highlighter()->setActive(enabled);
    if (enabled) {
        highlighter()->slotRehighlight();
    }
}

void MultiLineEdit::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu* menu = createStandardContextMenu();
    menu->addSeparator();

    auto action = menu->addAction(tr("Auto Spell Check"));
    action->setCheckable(true);
    action->setChecked(highlighter()->isActive());
    connect(action, &QAction::toggled, this, &MultiLineEdit::setSpellCheckEnabled);

    menu->exec(event->globalPos());
    delete menu;
}

#endif

void MultiLineEdit::setCustomFont(const QFont& font)
{
    setFont(font);
    updateSizeHint();
}

void MultiLineEdit::setMode(Mode mode)
{
    if (mode == _mode)
        return;

    _mode = mode;
}

void MultiLineEdit::setLineWrapEnabled(bool enable)
{
    setLineWrapMode(enable ? WidgetWidth : NoWrap);
    updateSizeHint();
}

void MultiLineEdit::setMinHeight(int lines)
{
    if (lines == _minHeight)
        return;

    _minHeight = lines;
    updateSizeHint();
}

void MultiLineEdit::setMaxHeight(int lines)
{
    if (lines == _maxHeight)
        return;

    _maxHeight = lines;
    updateSizeHint();
}

void MultiLineEdit::setScrollBarsEnabled(bool enable)
{
    if (_scrollBarsEnabled == enable)
        return;

    _scrollBarsEnabled = enable;
    updateScrollBars();
}

void MultiLineEdit::updateScrollBars()
{
    QFontMetrics fm(font());
    int _maxPixelHeight = fm.lineSpacing() * _maxHeight;
    if (_scrollBarsEnabled && document()->size().height() > _maxPixelHeight)
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    else
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    if (!_scrollBarsEnabled || isSingleLine())
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    else
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void MultiLineEdit::resizeEvent(QResizeEvent* event)
{
    QTextEdit::resizeEvent(event);
    updateSizeHint();
    updateScrollBars();
}

void MultiLineEdit::updateSizeHint()
{
    QFontMetrics fm(font());
    int minPixelHeight = fm.lineSpacing() * _minHeight;
    int maxPixelHeight = fm.lineSpacing() * _maxHeight;
    int scrollBarHeight = horizontalScrollBar()->isVisible() ? horizontalScrollBar()->height() : 0;

    // use the style to determine a decent size
    int h = qMin(qMax((int)document()->size().height() + scrollBarHeight, minPixelHeight), maxPixelHeight) + 2 * frameWidth();

    QStyleOptionFrame opt;
    opt.initFrom(this);
    opt.rect = QRect(0, 0, 100, h);
    opt.lineWidth = lineWidth();
    opt.midLineWidth = midLineWidth();
    opt.state |= QStyle::State_Sunken;
    QWidget* widget = this;
    QSize s = style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(100, h), widget);
    if (s != _sizeHint) {
        _sizeHint = s;
        updateGeometry();
    }
}

QSize MultiLineEdit::sizeHint() const
{
    if (!_sizeHint.isValid()) {
        auto* that = const_cast<MultiLineEdit*>(this);
        that->updateSizeHint();
    }
    return _sizeHint;
}

QSize MultiLineEdit::minimumSizeHint() const
{
    return sizeHint();
}

void MultiLineEdit::setEmacsMode(bool enable)
{
    _emacsMode = enable;
}

void MultiLineEdit::setPasteProtectionEnabled(bool enable, QWidget*)
{
    _pasteProtectionEnabled = enable;
}

void MultiLineEdit::historyMoveBack()
{
    addToHistory(convertRichtextToMircCodes(), true);

    if (_idx > 0) {
        _idx--;
        showHistoryEntry();
    }
}

void MultiLineEdit::historyMoveForward()
{
    addToHistory(convertRichtextToMircCodes(), true);

    if (_idx < _history.count()) {
        _idx++;
        if (_idx < _history.count() || _tempHistory.contains(_idx))  // tempHistory might have an entry for idx == history.count() + 1
            showHistoryEntry();
        else
            reset();  // equals clear() in this case
    }
    else {
        addToHistory(convertRichtextToMircCodes());
        reset();
    }
}

bool MultiLineEdit::addToHistory(const QString& text, bool temporary)
{
    if (text.isEmpty())
        return false;

    Q_ASSERT(0 <= _idx && _idx <= _history.count());

    if (temporary) {
        // if an entry of the history is changed, we remember it and show it again at this
        // position until a line was actually sent
        // sent lines get appended to the history
        if (_history.isEmpty() || text != _history[_idx - (int)(_idx == _history.count())]) {
            _tempHistory[_idx] = text;
            return true;
        }
    }
    else {
        if (_history.isEmpty() || text != _history.last()) {
            _history << text;
            _tempHistory.clear();
            return true;
        }
    }
    return false;
}

bool MultiLineEdit::event(QEvent* e)
{
    // We need to make sure that global shortcuts aren't eaten
    if (e->type() == QEvent::ShortcutOverride) {
        auto* event = static_cast<QKeyEvent*>(e);
        QKeySequence key = QKeySequence(event->key() | event->modifiers());
        foreach (QAction* action, GraphicalUi::actionCollection()->actions()) {
            if (action->shortcuts().contains(key)) {
                e->ignore();
                return false;
            }
        }
    }

    return MultiLineEditParent::event(e);
}

void MultiLineEdit::keyPressEvent(QKeyEvent* event)
{
    if (event == QKeySequence::InsertLineSeparator) {
        if (_mode == SingleLine) {
            event->accept();
            on_returnPressed();
            return;
        }
        MultiLineEditParent::keyPressEvent(event);
        return;
    }

    switch (event->key()) {
    case Qt::Key_Up:
        if (event->modifiers() & Qt::ShiftModifier)
            break;
        {
            event->accept();
            if (!(event->modifiers() & Qt::ControlModifier)) {
                int pos = textCursor().position();
                moveCursor(QTextCursor::Up);
                if (pos == textCursor().position())  // already on top line -> history
                    historyMoveBack();
            }
            else
                historyMoveBack();
            return;
        }

    case Qt::Key_Down:
        if (event->modifiers() & Qt::ShiftModifier)
            break;
        {
            event->accept();
            if (!(event->modifiers() & Qt::ControlModifier)) {
                int pos = textCursor().position();
                moveCursor(QTextCursor::Down);
                if (pos == textCursor().position())  // already on bottom line -> history
                    historyMoveForward();
            }
            else
                historyMoveForward();
            return;
        }

    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Select:
        event->accept();
        on_returnPressed();
        return;

    // We don't want to have the tab key react even if no completer is installed
    case Qt::Key_Tab:
        event->accept();
        return;

    default:;
    }

    if (_emacsMode) {
        if (event->modifiers() & Qt::ControlModifier) {
            switch (event->key()) {
            // move
            case Qt::Key_A:
                moveCursor(QTextCursor::StartOfLine);
                return;
            case Qt::Key_E:
                moveCursor(QTextCursor::EndOfLine);
                return;
            case Qt::Key_F:
                moveCursor(QTextCursor::Right);
                return;
            case Qt::Key_B:
                moveCursor(QTextCursor::Left);
                return;

            // modify
            case Qt::Key_Y:
                paste();
                return;
            case Qt::Key_K:
                moveCursor(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
                cut();
                return;

            default:
                break;
            }
        }
        else if (event->modifiers() & Qt::MetaModifier || event->modifiers() & Qt::AltModifier) {
            switch (event->key()) {
            case Qt::Key_Right:
                moveCursor(QTextCursor::WordRight);
                return;
            case Qt::Key_Left:
                moveCursor(QTextCursor::WordLeft);
                return;
            case Qt::Key_F:
                moveCursor(QTextCursor::WordRight);
                return;
            case Qt::Key_B:
                moveCursor(QTextCursor::WordLeft);
                return;
            case Qt::Key_Less:
                moveCursor(QTextCursor::Start);
                return;
            case Qt::Key_Greater:
                moveCursor(QTextCursor::End);
                return;

            // modify
            case Qt::Key_D:
                moveCursor(QTextCursor::WordRight, QTextCursor::KeepAnchor);
                cut();
                return;

            case Qt::Key_U:  // uppercase word
                moveCursor(QTextCursor::WordRight, QTextCursor::KeepAnchor);
                textCursor().insertText(textCursor().selectedText().toUpper());
                return;

            case Qt::Key_L:  // lowercase word
                moveCursor(QTextCursor::WordRight, QTextCursor::KeepAnchor);
                textCursor().insertText(textCursor().selectedText().toLower());
                return;

            case Qt::Key_C: {  // capitalize word
                moveCursor(QTextCursor::WordRight, QTextCursor::KeepAnchor);
                QString const text = textCursor().selectedText();
                textCursor().insertText(text.left(1).toUpper() + text.mid(1).toLower());
                return;
            }

            case Qt::Key_T: {  // transpose words
                moveCursor(QTextCursor::StartOfWord);
                moveCursor(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
                QString const word1 = textCursor().selectedText();
                textCursor().clearSelection();
                moveCursor(QTextCursor::WordRight);
                moveCursor(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
                QString const word2 = textCursor().selectedText();
                if (!word2.isEmpty() && !word1.isEmpty()) {
                    textCursor().insertText(word1);
                    moveCursor(QTextCursor::WordLeft);
                    moveCursor(QTextCursor::WordLeft);
                    moveCursor(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
                    textCursor().insertText(word2);
                    moveCursor(QTextCursor::WordRight);
                    moveCursor(QTextCursor::EndOfWord);
                }
                return;
            }

            default:
                break;
            }
        }
    }

#ifdef HAVE_KDE
    KTextEdit::keyPressEvent(event);
#else
    QTextEdit::keyPressEvent(event);
#endif
}

QString MultiLineEdit::convertRichtextToMircCodes()
{
    bool underline, bold, italic, color, strikethrough;
    QString mircText, mircFgColor, mircBgColor;
    QTextCursor cursor = textCursor();
    QTextCursor peekcursor = textCursor();
    cursor.movePosition(QTextCursor::Start);

    underline = bold = italic = color = strikethrough = false;

    while (cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor)) {
        if (cursor.selectedText() == QString(QChar(QChar::LineSeparator))
            || cursor.selectedText() == QString(QChar(QChar::ParagraphSeparator))) {
            if (color) {
                color = false;
                mircText.append('\x03');
            }
            if (underline) {
                underline = false;
                mircText.append('\x1f');
            }
            if (italic) {
                italic = false;
                mircText.append('\x1d');
            }
            if (bold) {
                bold = false;
                mircText.append('\x02');
            }
            if (strikethrough) {
                strikethrough = false;
                mircText.append('\x1E');
            }
            mircText.append('\n');
        }
        else {
            if (!bold && cursor.charFormat().font().bold()) {
                bold = true;
                mircText.append('\x02');
            }
            if (!italic && cursor.charFormat().fontItalic()) {
                italic = true;
                mircText.append('\x1d');
            }
            if (!underline && cursor.charFormat().fontUnderline()) {
                underline = true;
                mircText.append('\x1f');
            }
            if (!strikethrough && cursor.charFormat().fontStrikeOut()) {
                strikethrough = true;
                mircText.append('\x1E');
            }
            if (!color && (cursor.charFormat().foreground().isOpaque() || cursor.charFormat().background().isOpaque())) {
                color = true;
                mircText.append('\x03');
                mircFgColor = _mircColorMap.key(cursor.charFormat().foreground().color().name());
                mircBgColor = _mircColorMap.key(cursor.charFormat().background().color().name());

                if (mircFgColor.isEmpty()) {
                    mircFgColor = "01";  // use black if the current foreground color can't be converted
                }

                mircText.append(mircFgColor);
                if (cursor.charFormat().background().isOpaque())
                    mircText.append("," + mircBgColor);
            }

            mircText.append(cursor.selectedText());

            peekcursor.setPosition(cursor.position());
            peekcursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);

            if (mircCodesChanged(cursor, peekcursor)) {
                if (color) {
                    color = false;
                    mircText.append('\x03');
                }
                if (underline) {
                    underline = false;
                    mircText.append('\x1f');
                }
                if (italic) {
                    italic = false;
                    mircText.append('\x1d');
                }
                if (bold) {
                    bold = false;
                    mircText.append('\x02');
                }
                if (strikethrough) {
                    strikethrough = false;
                    mircText.append('\x1E');
                }
            }
        }

        cursor.clearSelection();
    }

    if (color)
        mircText.append('\x03');

    if (underline)
        mircText.append('\x1f');

    if (italic)
        mircText.append('\x1d');

    if (bold)
        mircText.append('\x02');

    if (strikethrough)
        mircText.append('\x1E');

    return mircText;
}

bool MultiLineEdit::mircCodesChanged(QTextCursor& cursor, QTextCursor& peekcursor)
{
    bool changed = false;
    if (cursor.charFormat().font().bold() != peekcursor.charFormat().font().bold())
        changed = true;
    if (cursor.charFormat().fontItalic() != peekcursor.charFormat().fontItalic())
        changed = true;
    if (cursor.charFormat().fontUnderline() != peekcursor.charFormat().fontUnderline())
        changed = true;
    if (cursor.charFormat().fontStrikeOut() != peekcursor.charFormat().fontStrikeOut())
        changed = true;
    if (cursor.charFormat().foreground().color() != peekcursor.charFormat().foreground().color())
        changed = true;
    if (cursor.charFormat().background().color() != peekcursor.charFormat().background().color())
        changed = true;
    return changed;
}

QString MultiLineEdit::convertMircCodesToHtml(const QString& text)
{
    QStringList words;
    QRegularExpression mircCode = QRegularExpression("(\x02|\x1d|\x1f|\x03|\x1E)");

    int posLeft = 0;
    QRegularExpressionMatchIterator it = mircCode.globalMatch(text);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        int posRight = match.capturedStart();
        if (posLeft < posRight) {
            words << text.mid(posLeft, posRight - posLeft);
        }
        words << match.captured(0);
        posLeft = posRight + match.capturedLength(0);
    }

    if (posLeft < text.length()) {
        words << text.mid(posLeft);
    }

    for (int i = 0; i < words.count(); i++) {
        QString style;
        if (words[i].contains('\x02')) {
            style.append(" font-weight:600;");
            words[i].replace('\x02', "");
        }
        if (words[i].contains('\x1d')) {
            style.append(" font-style:italic;");
            words[i].replace('\x1d', "");
        }
        if (words[i].contains('\x1f')) {
            style.append(" text-decoration: underline;");
            words[i].replace('\x1f', "");
        }
        if (words[i].contains('\x1E')) {
            style.append(" text-decoration: line-through;");
            words[i].replace('\x1E', "");
        }
        if (words[i].contains('\x03')) {
            int pos = words[i].indexOf('\x03');
            int len = 3;
            QString fg = words[i].mid(pos + 1, 2);
            QString bg;
            if (words[i][pos + 3] == ',')
                bg = words[i].mid(pos + 4, 2);

            style.append(" color:");
            style.append(_mircColorMap[fg]);
            style.append(";");

            if (!bg.isEmpty()) {
                style.append(" background-color:");
                style.append(_mircColorMap[bg]);
                style.append(";");
                len = 6;
            }
            words[i].replace(pos, len, "");
            words[i].replace('\x03', "");
        }
        words[i].replace("&", "&");
        words[i].replace("<", "<");
        words[i].replace(">", ">");
        words[i].replace("\"", "");
        if (style.isEmpty()) {
            words[i] = "<span>" + words[i] + "</span>";
        }
        else {
            words[i] = "<span style=\"" + style + "\">" + words[i] + "</span>";
        }
    }
    return words.join("").replace("\n", "<br />");
}

void MultiLineEdit::on_returnPressed()
{
    on_returnPressed(convertRichtextToMircCodes());
}

void MultiLineEdit::on_returnPressed(QString text)
{
    if (_completionSpace && text.endsWith(" ")) {
        text.chop(1);
    }

    if (!text.isEmpty()) {
        foreach (const QString& line, text.split('\n', Qt::SkipEmptyParts)) {
            if (line.isEmpty())
                continue;
            addToHistory(line);
            emit textEntered(line);
        }
        reset();
        _tempHistory.clear();
    }
    else {
        emit noTextEntered();
    }
}

void MultiLineEdit::on_textChanged()
{
    _completionSpace = qMax(_completionSpace - 1, 0);

    QString newText = text();
    newText.replace("\r\n", "\n");
    newText.replace('\r', '\n');
    if (_mode == SingleLine) {
        if (!pasteProtectionEnabled())
            newText.replace('\n', ' ');
        else if (newText.contains('\n')) {
            QStringList lines = newText.split('\n', Qt::SkipEmptyParts);
            clear();

            if (lines.count() >= 4) {
                QString msg = tr("Do you really want to paste %n line(s)?", "", lines.count());
                msg += "<p>";
                for (int i = 0; i < 4; i++) {
                    msg += lines[i].left(40).toHtmlEscaped();
                    if (lines[i].length() > 40)
                        msg += "...";
                    msg += "<br />";
                }
                msg += "...</p>";
                QMessageBox question(QMessageBox::NoIcon, tr("Paste Protection"), msg, QMessageBox::Yes | QMessageBox::No);
                question.setDefaultButton(QMessageBox::No);
#ifdef Q_OS_MAC
                question.setWindowFlags(question.windowFlags() | Qt::Sheet);
#endif
                if (question.exec() != QMessageBox::Yes)
                    return;
            }

            foreach (QString line, lines) {
                clear();
                insert(line);
                on_returnPressed();
            }
        }
    }

    _singleLine = (newText.indexOf('\n') < 0);

    if (document()->size().height() != _lastDocumentHeight) {
        _lastDocumentHeight = document()->size().height();
        on_documentHeightChanged(_lastDocumentHeight);
    }
    updateSizeHint();
    ensureCursorVisible();
}

void MultiLineEdit::on_documentHeightChanged(qreal)
{
    updateScrollBars();
}

void MultiLineEdit::reset()
{
    // every time the MultiLineEdit is cleared we also reset history index
    _idx = _history.count();
    clear();
    QTextBlockFormat format = textCursor().blockFormat();
    format.setLeftMargin(leftMargin);  // we want a little space between the frame and the contents
    textCursor().setBlockFormat(format);
    updateScrollBars();
}

void MultiLineEdit::showHistoryEntry()
{
    // if the user changed the history, display the changed line
    setHtml(convertMircCodesToHtml(_tempHistory.contains(_idx) ? _tempHistory[_idx] : _history[_idx]));
    QTextCursor cursor = textCursor();
    QTextBlockFormat format = cursor.blockFormat();
    format.setLeftMargin(leftMargin);  // we want a little space between the frame and the contents
    cursor.setBlockFormat(format);
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
    updateScrollBars();
}

void MultiLineEdit::addCompletionSpace()
{
    // Inserting the space emits textChanged, which should not disable removal
    _completionSpace = 2;
    insertPlainText(" ");
}
