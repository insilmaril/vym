#ifndef SCRIPTOUTPUT_H
#define SCRIPTOUTPUT_H

#include <QTextEdit>
#include <QVBoxLayout>

class ScriptOutput : public QWidget {
    Q_OBJECT
  public:
    ScriptOutput(QWidget *parent);
    ~ScriptOutput();
    void setFocus();
    void clear();
    void setText(const QString &text);
    QString text();
    void append(const QString &text);

  public slots:
    void closeWindow();

  private:
    QTextEdit *editor;
    QVBoxLayout *layout;
};

#endif // SCRIPTOUTPUT_H
