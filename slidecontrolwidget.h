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

signals:
    void hideFindWidget();
    void takeSnapshot();
    void previousButtonPressed();
    void nextButtonPressed();

protected:
    QPushButton *previousButton;
    QPushButton *snapshotButton;
    QPushButton *nextButton;
};

#endif

