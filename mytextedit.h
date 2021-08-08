#ifndef MYTEXTEDIT_H
#define MYTEXTEDIT_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QCompleter>
#include <QStringList>
#include <QPair>
#include <QVector>

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
    QVector<QPair<int, int> > ipspan;
public:
    explicit MyTextEdit(QWidget *parent = nullptr);
    QString toPlainText();
    void setPlainText(QString);
    void setPlaceholderText(QString);
    void setCompleter(QStringList );
private:
    QString textUnderCursor();
    bool eventFilter(QObject *, QEvent *);

private slots:
    void changeCompletion(QString);
public slots:

    void complete();
signals:
};

#endif // MYTEXTEDIT_H
