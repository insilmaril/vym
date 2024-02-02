#ifndef SLIDECONTROLWIDGET_H
#define SLIDECONTROLWIDGET_H

#include <QWidget>

class QAction;
class QPushButton;

class SlideControlWidget : public QWidget {
    Q_OBJECT

  public:
    SlideControlWidget(QWidget *parent = nullptr);

  public slots:
    void previousPressed();
    void snapshotPressed();
    void nextPressed();
    void upPressed();
    void downPressed();
    void deletePressed();
    void editPressed();

  signals:
    void hideFindWidget();
    void takeSnapshot();
    void deleteButtonPressed();
    void editButtonPressed();
    void previousButtonPressed();
    void nextButtonPressed();
    void upButtonPressed();
    void downButtonPressed();

  protected:
    QPushButton *previousButton;
    QPushButton *snapshotButton;
    QPushButton *editButton;
    QPushButton *deleteButton;
    QPushButton *nextButton;
    QPushButton *upButton;
    QPushButton *downButton;
};

#endif
