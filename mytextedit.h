#ifndef MYTEXTEDIT_H
#define MYTEXTEDIT_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QCompleter>
#include <QStringList>

class Completer: public QCompleter
{
Q_OBJECT
public:
    explicit Completer(QWidget *parent= nullptr);
};

class MyTextEdit : public QWidget
{
    Q_OBJECT
private:
    QPlainTextEdit *edit;
    Completer *completer;
public:
    explicit MyTextEdit(QWidget *parent = nullptr);
    QString toPlainText();
    void setPlainText(QString);
    void setPlaceholderText(QString);
    void setCompleter(QStringList );

signals:
};

#endif // MYTEXTEDIT_H
