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
  
  private:  
    QAction *actionOpenUrl;
    QAction *actionEditUrl;
    QPoint lastContextMenuPositionInt;

  private slots:  
    void openUrlTriggered();
    void editUrlTriggered();

  signals:  
    void editUrlCursor(QTextCursor);
};

#endif
