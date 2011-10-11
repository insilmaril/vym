#include <QAction>
#include <QDebug>
#include <QLineEdit>
#include <QVBoxLayout>

#include <QPushButton>
#include <QLabel>


#include "slidecontrolwidget.h"
#include "mainwindow.h"


extern QString iconPath;
extern Main *mainWindow;

SlideControlWidget::SlideControlWidget(QWidget *)
{
    QVBoxLayout* mainLayout = new QVBoxLayout;	
    QHBoxLayout *row2Layout = new QHBoxLayout;
    
    previousButton = new QPushButton;
 //   previousButton->setIcon (QPixmap(iconPath+"SlideControl.png"));
    previousButton->setText (tr("Previous slide","SlideControl widget"));
    connect ( previousButton, SIGNAL( clicked() ), this, SLOT( previousPressed() ) );

    nextButton = new QPushButton;
 //   nextButton->setIcon (QPixmap(iconPath+"SlideControl.png"));
    nextButton->setText (tr("Next slide","SlideControl widget"));
    connect ( nextButton, SIGNAL( clicked() ), this, SLOT( nextPressed() ) );

    // QAction needed to only activate shortcut while SlideControlWidget has focus
 //   a=new QAction (nextButton->text(),this);
//    a->setShortcut (Qt::Key_Return);
//    a->setShortcutContext (Qt::WidgetWithChildrenShortcut);
//    connect ( a, SIGNAL( triggered() ), this, SLOT( nextPressed() ) );
//    addAction (a);

 //   connect (     nextButton, SIGNAL( clicked() ), this, SLOT(     nextPressed() ) );

    // QAction needed to only activate shortcut while SlideControlWidget has focus
  //  QAction *a=new QAction (previousButton->text(),this);
//    a->setShortcut (Qt::Key_Return);
//    a->setShortcutContext (Qt::WidgetWithChildrenShortcut);
//    addAction (a);

    snapshotButton = new QPushButton;
 //   snapshotButton->setIcon (QPixmap(iconPath+"SlideControl.png"));
    snapshotButton->setText (tr("Take snapshot","SlideControl widget"));
    connect ( snapshotButton, SIGNAL( clicked() ), this, SLOT( snapshotPressed() ) );

    // QAction needed to only activate shortcut while SlideControlWidget has focus
//    a=new QAction (snapshotButton->text(),this);
//    a->setShortcut (Qt::Key_Return);
//    a->setShortcutContext (Qt::WidgetWithChildrenShortcut);
//    connect ( a, SIGNAL( triggered() ), this, SLOT( snapshotPressed() ) );
//    addAction (a);

    row2Layout->addWidget(previousButton);
    row2Layout->addWidget(snapshotButton);
    row2Layout->addWidget(nextButton);

    mainLayout->addLayout (row2Layout);

    setLayout (mainLayout);
}

void SlideControlWidget::snapshotPressed()
{
    emit (takeSnapshot() );
}

void SlideControlWidget::previousPressed()
{
    emit (previousButtonPressed() );
}

void SlideControlWidget::nextPressed()
{
    emit (nextButtonPressed() );
}


