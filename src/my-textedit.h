#ifndef MY_TEXTEDIT_H
#define MY_TEXTEDIT_H

#include <QTextEdit>

class MyTextEdit : public QTextEdit {
    Q_OBJECT
  public:
    MyTextEdit(QWidget *parent = nullptr);

  protected:
    void mousePressEvent(QMouseEvent *);
    void contextMenuEvent(QContextMenuEvent *);

  private:
    QAction *actionOpenUrl;
    QPoint lastContextMenuPos;

  private slots:  
    void openUrlTriggered();
};

#endif
