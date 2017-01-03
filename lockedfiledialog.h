#ifndef LOCKEDFILEDIALOG_H
#define LOCKEDFILEDIALOG_H

#include "ui_lockedfiledialog.h"

class LockedFileDialog : public QDialog
{
    Q_OBJECT

public:
    enum Result {OpenReadonly, DeleteLockfile};

    LockedFileDialog(QWidget* parent = 0);
    Result execDialog();

public slots:
    virtual void setText( const QString & s );
    virtual void setCaption( const QString & s );

private:
    void init();
    Ui::LockedFileDialog ui;
};

#endif 
