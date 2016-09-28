#include <QAction>
#include <QDebug>
#include <QLineEdit>
#include <QVBoxLayout>

#include <QPushButton>
#include <QLabel>


#include "slidecontrolwidget.h"
#include "mainwindow.h"


extern Main *mainWindow;

SlideControlWidget::SlideControlWidget(QWidget *)
{
    QVBoxLayout* mainLayout = new QVBoxLayout;	
    QHBoxLayout *row2Layout = new QHBoxLayout;
    
    previousButton = new QPushButton;
    previousButton->setIcon ( QPixmap( ":/slideprevious.png") );
    connect ( previousButton, SIGNAL( clicked() ), this, SLOT( previousPressed() ) );

    nextButton = new QPushButton;
    nextButton->setIcon ( QPixmap( ":/slidenext.png") );
    connect ( nextButton, SIGNAL( clicked() ), this, SLOT( nextPressed() ) );

    upButton = new QPushButton;
    upButton->setIcon ( QPixmap( ":/up.png") );
    connect ( upButton, SIGNAL( clicked() ), this, SLOT( upPressed() ) );

    downButton = new QPushButton;
    downButton->setIcon ( QPixmap( ":/down.png") );
    connect ( downButton, SIGNAL( clicked() ), this, SLOT( downPressed() ) );

    snapshotButton = new QPushButton;
    //snapshotButton->setIcon (QPixmap ( ":/sliderecord.png" ));
    // Original: /usr/share/icons/oxygen/32x32/devices/camera-photo.png
    snapshotButton->setIcon (QPixmap ( ":/slide-camera.png" ));
    connect ( snapshotButton, SIGNAL( clicked() ), this, SLOT( snapshotPressed() ) );

    editButton = new QPushButton;
    editButton->setIcon (QPixmap ( ":/scripteditor.png" ));   
    connect ( editButton, SIGNAL( clicked() ), this, SLOT( editPressed() ) );

    deleteButton = new QPushButton;
    deleteButton->setIcon (QPixmap ( ":/edittrash.png" ));
    connect ( deleteButton, SIGNAL( clicked() ), this, SLOT( deletePressed() ) );

    row2Layout->addWidget(previousButton);
    row2Layout->addWidget(nextButton);
    row2Layout->addWidget(snapshotButton);
    row2Layout->addWidget(editButton);
    row2Layout->addWidget(deleteButton);
    row2Layout->addWidget(upButton);
    row2Layout->addWidget(downButton);

    mainLayout->addLayout (row2Layout);

    setLayout (mainLayout);
}

void SlideControlWidget::snapshotPressed()
{
    emit (takeSnapshot() );
}

void SlideControlWidget::editPressed()
{
    emit (editButtonPressed() );
}

void SlideControlWidget::deletePressed()
{
    emit (deleteButtonPressed() );
}

void SlideControlWidget::previousPressed()
{
    emit (previousButtonPressed() );
}

void SlideControlWidget::nextPressed()
{
    emit (nextButtonPressed() );
}

void SlideControlWidget::upPressed()
{
    emit (upButtonPressed() );
}

void SlideControlWidget::downPressed()
{
    emit (downButtonPressed() );
}


