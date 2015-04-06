#ifndef SCRIPTOUTPUT_H
#define SCRIPTOUTPUT_H

#include <QTextEdit>
#include <QVBoxLayout>

class ScriptOutput:public QWidget
{
    Q_OBJECT
public:
    ScriptOutput(QWidget *parent = 0);
    ~ScriptOutput();
    void setText(const QString &text);
    void append(const QString &text);

private:
    QTextEdit *editor;
    QVBoxLayout *layout;
};

#endif // SCRIPTOUTPUT_H
