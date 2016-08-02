#ifndef BRANCHPROPERTYEDITOR_H
#define BRANCHPROPERTYEDITOR_H

#include "ui_branchpropeditor.h"

#include <QDialog>
#include <QCloseEvent>
#include <QtGui>


//#include "attributedelegate.h"
#include "branchobj.h"

class QAbstractItemModel;
class VymModel;

class BranchPropertyEditor:public QDialog
{
    Q_OBJECT
public:
    BranchPropertyEditor (QWidget *parent=0);
    ~BranchPropertyEditor ();
    void setItem (TreeItem *);
    void setModel (VymModel *);

private slots:
    void frameTypeChanged (int);
    void framePenColorClicked ();
    void frameBrushColorClicked ();
    void framePaddingChanged(int);
    void frameBorderWidthChanged(int);
    void frameIncludeChildrenChanged(int);
    void linkHideUnselectedChanged (int);
    void incImgVerChanged (int);
    void incImgHorChanged (int);
    void childrenFreePositioningChanged (int);
//  void addAttributeClicked();
//  void deleteAttributeClicked();

signals:
    void windowClosed();
    
protected:
    void closeEvent( QCloseEvent* );

private:
    void connectSignals();
    void disconnectSignals();
    
    Ui::BranchPropertyEditor ui;

    BranchObj *branchObj;
    BranchItem *branchItem;
    VymModel *model;

    QColor penColor;
    QColor brushColor;

//  QAbstractItemModel *attributeModel;

//  AttributeDelegate delegate;
};

#endif // 
