#ifndef ATTRIBUTEDIALOG_H
#define ATTRIBUTEDIALOG_H

#include "attribute.h"
#include "branchobj.h"

#include <QDialog>
#include <QCloseEvent>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>


/*! \brief Set the dialog mode  */
enum AttributeDialogMode {
    Definition,	//!< Edit attribute definitions (AttributeDef)
    Object	//!< Edit attributes of OrnamentedObj
};

class AttributeDialog:public QDialog
{
    Q_OBJECT 
public:
    AttributeDialog (QWidget *parent=0 );
    void setTable (AttributeTable *table=0);
    void setBranch (BranchObj *bo);
    void setMode (const AttributeDialogMode &m);
    void updateTable();
signals:
    void windowClosed();
private slots:
    void addKey();
protected:
    void closeEvent(QCloseEvent*);
private:
    QVBoxLayout *vboxLayout;
    QVBoxLayout *tableLayout;
    QHBoxLayout *hboxLayout;
    QPushButton *addButton;
    QSpacerItem *spacerItem;
    QPushButton *closeButton;

    AttributeDialogMode mode;
    AttributeTable *table;
    BranchObj *branch;
};

#endif
