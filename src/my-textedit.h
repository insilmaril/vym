#ifndef MY_TEXTEDIT_H
#define MY_TEXTEDIT_H

#include <QTextEdit>

class MyTextEdit : public QTextEdit {
    Q_OBJECT

  public:
    MyTextEdit(QWidget *parent = nullptr);
    bool richTextMode();
    void setRichTextMode(bool b);

  protected:
    void mousePressEvent(QMouseEvent *);
    void contextMenuEvent(QContextMenuEvent *);

  private:
    bool richTextModeInt;
    QAction *actionOpenUrl;
    QPoint lastContextMenuPos;

  private slots:  
    void openUrlTriggered();
};

#endif
