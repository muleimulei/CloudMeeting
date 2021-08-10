#include "mytextedit.h"
#include <QVBoxLayout>
#include <QStringListModel>
#include <QDebug>
#include <QAbstractItemView>
#include <QScrollBar>

Completer::Completer(QWidget *parent): QCompleter(parent)
{

}

MyTextEdit::MyTextEdit(QWidget *parent): QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    edit = new QPlainTextEdit();
    edit->setPlaceholderText(QString::fromUtf8("&#x8F93;&#x5165;@&#x53EF;&#x4EE5;&#x5411;&#x5BF9;&#x5E94;&#x7684;IP&#x53D1;&#x9001;&#x6570;&#x636E;"));
    layout->addWidget(edit);
    completer = nullptr;
    connect(edit, SIGNAL(textChanged()), this, SLOT(complete()));
    edit->installEventFilter(this);
}

QString MyTextEdit::textUnderCursor()
{
    QTextCursor tc = edit->textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}

void MyTextEdit::complete()
{
    if(edit->toPlainText().size() == 0 || completer == nullptr) return;
    QChar tail =  edit->toPlainText().at(edit->toPlainText().size()-1);
    if(tail == '@')
    {
        completer->setCompletionPrefix(tail);
        QAbstractItemView *view = completer->popup();
        view->setCurrentIndex(completer->completionModel()->index(0, 0));
        QRect cr = edit->cursorRect();
        QScrollBar *bar = completer->popup()->verticalScrollBar();
        cr.setWidth(completer->popup()->sizeHintForColumn(0) + bar->sizeHint().width());
        completer->complete(cr);
    }
}

void MyTextEdit::changeCompletion(QString text)
{
    QTextCursor tc = edit->textCursor();
    int len = text.size() - completer->completionPrefix().size();
    tc.movePosition(QTextCursor::EndOfWord);
    tc.insertText(text.right(len));
    edit->setTextCursor(tc);
    completer->popup()->hide();

    QString str = edit->toPlainText();
    int pos = str.size() - 1;
    while(str.at(pos) != '@') pos--;

    tc.clearSelection();
    tc.setPosition(pos, QTextCursor::MoveAnchor);
    tc.setPosition(str.size(), QTextCursor::KeepAnchor);
      // tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, str.size() - pos);

    QTextCharFormat fmt = tc.charFormat();
    QTextCharFormat fmt_back = fmt;
    fmt.setForeground(QBrush(Qt::white));
    fmt.setBackground(QBrush(QColor(0, 160, 233)));
    tc.setCharFormat(fmt);
    tc.clearSelection();
    tc.setCharFormat(fmt_back);

    tc.insertText(" ");
    edit->setTextCursor(tc);

    ipspan.push_back(QPair<int, int>(pos, str.size()+1));

}

QString MyTextEdit::toPlainText()
{
    return edit->toPlainText();
}

void MyTextEdit::setPlainText(QString str)
{
    edit->setPlainText(str);
}

void MyTextEdit::setPlaceholderText(QString str)
{
    edit->setPlaceholderText(str);
}

void MyTextEdit::setCompleter(QStringList stringlist)
{
    if(completer == nullptr)
    {
        completer = new Completer(this);
        completer->setWidget(this);
        completer->setCompletionMode(QCompleter::PopupCompletion);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        connect(completer, SIGNAL(activated(QString)), this, SLOT(changeCompletion(QString)));
    }
    else
    {
        delete completer->model();
    }
    QStringListModel * model = new QStringListModel(stringlist, this);
    completer->setModel(model);
}

bool MyTextEdit::eventFilter(QObject *obj, QEvent *event)
{
    if(obj == edit)
    {
        if(event->type() == QEvent::KeyPress)
        {
            QKeyEvent *keyevent = static_cast<QKeyEvent *>(event);
                QTextCursor tc = edit->textCursor();
                int p = tc.position();
                int i;
                for(i = 0; i < ipspan.size(); i++)
                {
                    if( (keyevent->key() == Qt::Key_Backspace && p > ipspan[i].first && p <= ipspan[i].second ) || (keyevent->key() == Qt::Key_Delete && p >= ipspan[i].first && p < ipspan[i].second) )
                    {
                        tc.setPosition(ipspan[i].first, QTextCursor::MoveAnchor);
                        if(p == ipspan[i].second)
                        {
                            tc.setPosition(ipspan[i].second, QTextCursor::KeepAnchor);
                        }
                        else
                        {
                            tc.setPosition(ipspan[i].second + 1, QTextCursor::KeepAnchor);
                        }
                        tc.removeSelectedText();
                        ipspan.removeAt(i);
                        return true;
                    }
                    else if(p >= ipspan[i].first && p <= ipspan[i].second)
                    {
                        QTextCursor tc = edit->textCursor();
                        tc.setPosition(ipspan[i].second);
                        edit->setTextCursor(tc);
                    }
                }
        }
    }
    return QWidget::eventFilter(obj, event);
}
