#ifndef EDITXLINKDIALOG_H
#define EDITXLINKDIALOG_H

#include "ui_editxlinkdialog.h"

/*! \brief Dialog to edit or delete XLinks 

Using xLinks one can link one branch to any other, just like attaching a rope
between two branches in a real tree. This is especially useful in complex maps,
where you want to have crossreferences which don't fit on the same visible area
*/

class BranchItem;

class EditXLinkDialog:public QDialog
{
    Q_OBJECT
public:
    EditXLinkDialog (QWidget *parent=0);
    void setLink (Link *);
    void setSelection (BranchItem*);
    bool useSettingsGlobal();
    bool deleteXLink();

private slots:
    void deleteButtonPressed();
    void widthChanged (int);
    void colorButtonPressed();
    void setColorHeadingButtonPressed ();

private:
    Ui::EditXLinkDialog ui;
    bool delink;
    Link *link;
    BranchItem *selbi;
};

#endif // EDITXLINKDIALOG_H
