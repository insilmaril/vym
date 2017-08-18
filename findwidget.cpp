#include <QAction>
#include <QDebug>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QLabel>

#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>


#include "findwidget.h"
#include "mainwindow.h"


extern Main *mainWindow;

FindWidget::FindWidget(QWidget *)
{
    QVBoxLayout* mainLayout = new QVBoxLayout;	
    QHBoxLayout *row2Layout = new QHBoxLayout;
    
    QLabel *label=new QLabel;
    label->setText (tr("Find:","FindWidget"));
    
    // Create LineEdit (here QComboBox)
    findcombo = new QComboBox;
    findcombo->setMinimumWidth(250);
    findcombo->setEditable(true);

    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    findcombo->setSizePolicy(sizePolicy);
    connect ( findcombo, SIGNAL( highlighted(int) ), 
	this, SLOT( nextPressed() ) );
    connect ( findcombo, SIGNAL( editTextChanged(const QString &) ), 
	this, SLOT( findTextChanged(const QString&) ) );

    nextbutton = new QPushButton;
    nextbutton->setIcon (QPixmap(":/find.png"));
    //nextbutton->setText (tr("Find","Find widget"));
    connect ( nextbutton, SIGNAL( clicked() ), this, SLOT( nextPressed() ) );

    // QAction needed to only activate shortcut while FindWidget has focus
    QAction *a=new QAction (nextbutton->text(),this);
    a->setShortcut (Qt::Key_Return);
    a->setShortcutContext (Qt::WidgetWithChildrenShortcut);
    connect ( a, SIGNAL( triggered() ), this, SLOT( nextPressed() ) );
    addAction (a);

    QPushButton *filterNotesButton = new QPushButton;
    filterNotesButton->setIcon (QPixmap(":/flag-note.png"));

    row2Layout->addWidget (label);
    row2Layout->addWidget(findcombo);
    row2Layout->addWidget(nextbutton);
    row2Layout->addWidget(filterNotesButton);

    mainLayout->addLayout (row2Layout);

    setLayout (mainLayout);
    status=Undefined;
}

QString FindWidget::getFindText()
{
    return findcombo->currentText();
}

void FindWidget::cancelPressed()
{
    hide();
    emit (hideFindWidget() );//Restore focus
}

void FindWidget::nextPressed()
{
    emit (nextButton(findcombo->currentText() ) );
}

void FindWidget::findTextChanged(const QString&)
{
    setStatus (Undefined);
}

void FindWidget::setFocus()
{
    findcombo->lineEdit()->selectAll();
    findcombo->lineEdit()->setFocus();
}

void FindWidget::setStatus (Status st)
{
    if (st==status) return;

    status=st;
    QPalette p=palette();
    QColor c;
    switch (st)
    {
	case Success: c=QColor (120,255,120); break;
	case Failed:  c=QColor (255,120,120); break;
	default:  c=QColor (255,255,255); 
    }
    p.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(9), c);
    p.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(9), c);
    findcombo->setPalette(p);
}

