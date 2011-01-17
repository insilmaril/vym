#include "historywindow.h"
#include "mainwindow.h"


extern QString iconPath;
extern Settings settings;
extern Main *mainWindow;

HistoryWindow::HistoryWindow (QWidget *parent):QDialog (parent)
{
    ui.setupUi (this);
    ui.historyTable->setRowCount (settings.value( "/mapeditor/stepsTotal",75).toInt());
    ui.historyTable->setColumnCount (3);

    
    QTableWidgetItem *item;

    item= new QTableWidgetItem(tr("Action","Table with actions"));
    ui.historyTable->setHorizontalHeaderItem(0, item);
    
    item= new QTableWidgetItem(tr("Comment","Table with actions"));
    ui.historyTable->setHorizontalHeaderItem(1, item);
    
    item= new QTableWidgetItem(tr("Undo action","Table with actions"));
    ui.historyTable->setHorizontalHeaderItem(2, item);

    ui.historyTable->setSelectionBehavior (QAbstractItemView::SelectRows);

    ui.undoButton->setIcon (QIcon(iconPath+"/undo.png"));
    ui.redoButton->setIcon (QIcon(iconPath+"/redo.png"));

    connect ( ui.undoButton, SIGNAL (clicked()), this, SLOT (undo()));
    connect ( ui.redoButton, SIGNAL (clicked()), this, SLOT (redo()));
    connect ( ui.historyTable, SIGNAL (itemSelectionChanged()), this, SLOT (select()));

    // Load Settings

    resize (settings.value ( "/satellite/historywindow/geometry/size", QSize(1000,400)).toSize());
    move   (settings.value ( "/satellite/historywindow/geometry/pos", QPoint (0,450)).toPoint());

    ui.historyTable->setColumnWidth (0,settings.value("/satellite/historywindow/geometry/columnWidth/0",250).toInt());
    ui.historyTable->setColumnWidth (1,settings.value("/satellite/historywindow/geometry/columnWidth/1",350).toInt());
    ui.historyTable->setColumnWidth (2,settings.value("/satellite/historywindow/geometry/columnWidth/2",250).toInt());
}

HistoryWindow::~HistoryWindow()
{
    // Save settings
    settings.setValue( "/satellite/historywindow/geometry/size", size() );
    settings.setValue( "/satellite/historywindow/geometry/pos", pos() );

    for (int i=0; i<3; ++i)
	settings.setValue( QString("/satellite/historywindow/geometry/columnWidth/%1").arg(i), ui.historyTable->columnWidth (i) );
}

void HistoryWindow::clearRow(int row)
{
    QTableWidgetItem *it;
    it=ui.historyTable->item (row,0);
    if (it) it->setText ("");
    it=ui.historyTable->item (row,1);
    if (it) it->setText ("");
    it=ui.historyTable->item (row,2);
    if (it) it->setText ("");
}

void HistoryWindow::updateRow(int row, int step, SimpleSettings &set)
{
    QTableWidgetItem *item;

    item= new QTableWidgetItem(set.value(QString("/history/step-%1/redoCommand").arg(step)));
    ui.historyTable->setItem(row, 0, item);

    item= new QTableWidgetItem(set.value(QString("/history/step-%1/comment").arg(step)));
    ui.historyTable->setItem(row, 1, item);

    item=new QTableWidgetItem(set.value(QString("/history/step-%1/undoCommand").arg(step)));
    ui.historyTable->setItem(row, 2, item);
}

void HistoryWindow::update(SimpleSettings &set)
{
    int undosAvail=set.readNumValue("/history/undosAvail",0);
    int redosAvail=set.readNumValue("/history/redosAvail",0);
    int stepsTotal=set.readNumValue("/history/stepsTotal",0);
    int curStep=set.readNumValue ("/history/curStep");
    int i;
    int s=curStep;
    int r=undosAvail-1;
    QTableWidgetItem *item;

    // Update number of rows
    ui.historyTable->setRowCount (undosAvail + redosAvail +1);

    // Update buttons
    if (undosAvail<1)
	ui.undoButton->setEnabled (false);
    else    
	ui.undoButton->setEnabled (true);

    if (redosAvail<1)
	ui.redoButton->setEnabled (false);
    else    
	ui.redoButton->setEnabled (true);

    // Update undos in table
    for (i=undosAvail; i>0; i--)
    {
	updateRow (r,s,set);
	r--;
	s--;
	if (s<1) s=stepsTotal;
    }
    
    // Generated the "now" row
    QColor c(255,200,120);
    for (i=0;i<=2;i++)
    {
	if (i!=1)
	{
	    item=new QTableWidgetItem("");
	    item->setBackgroundColor (c);
	    ui.historyTable->setItem(undosAvail, i, item);
	}
    }
    item=new QTableWidgetItem(" - " +tr("Current state","Current bar in history hwindow")+ " - ");
    item->setBackgroundColor (c);
    ui.historyTable->setItem(undosAvail, 1, item);

    // Show "now" row
    ui.historyTable->scrollToItem (item);

    // Update Redos in table
    s=curStep;
    s++; if (s>stepsTotal) s=1;
    for (i=1;i<= redosAvail; i++)
    {
	updateRow (undosAvail+i,s,set);
	s++; if (s>stepsTotal) s=1;
    }

    // Delete the rest
    for (i=undosAvail+redosAvail+1;i<= stepsTotal; i++)
	clearRow (i);

    //ui.historyTable->resizeColumnsToContents();
}

void HistoryWindow::setStepsTotal (int st)
{
    // Number of steps + "current" bar
    ui.historyTable->setRowCount (st+1);
}


void HistoryWindow::closeEvent (QCloseEvent *)
{
    hide();
    emit (windowClosed() );
}

void HistoryWindow::undo()
{
    mainWindow->editUndo();
}

void HistoryWindow::redo()
{
    mainWindow->editRedo();
}

void HistoryWindow::select()
{
    mainWindow->gotoHistoryStep (ui.historyTable->row (ui.historyTable->selectedItems().first()));
}
