#ifndef SLIDECONTROLWIDGET_H 
#define SLIDECONTROLWIDGET_H

#include <QWidget>

class QAction;
class QPushButton;

class SlideControlWidget: public QWidget
{
    Q_OBJECT

public:
    SlideControlWidget (QWidget *parent=NULL);

public slots:	
    void previousPressed();
    void snapshotPressed();
    void nextPressed();
    void upPressed();
    void downPressed();
    void deletePressed();

signals:
    void hideFindWidget();
    void takeSnapshot();
    void deleteButtonPressed();
    void previousButtonPressed();
    void nextButtonPressed();
    void upButtonPressed();
    void downButtonPressed();

protected:
    QPushButton *previousButton;
    QPushButton *snapshotButton;
    QPushButton *deleteButton;
    QPushButton *nextButton;
    QPushButton *upButton;
    QPushButton *downButton;
};

#endif

