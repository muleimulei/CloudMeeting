#include "mytextedit.h"
#include <QVBoxLayout>
#include <QStringListModel>
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
    }
    else
    {
        delete completer->model();
    }
    QStringListModel * model = new QStringListModel(stringlist, this);
    completer->setModel(model);
}
