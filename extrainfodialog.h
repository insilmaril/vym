#ifndef EXTRAINFODIALOG_H
#define EXTRAINFODIALOG_H

#include "ui_extrainfodialog.h"

/* \brief Dialog to display and edit map specific information like author, comment, etc. 
*/

class ExtraInfoDialog : public QDialog
{
    Q_OBJECT

public:
    ExtraInfoDialog(QWidget* parent = 0);

    virtual QString getComment();
    virtual QString getAuthor();
    virtual QString getMapTitle();
    bool lockFileUsed();
    virtual void setStats( const QString & s );

public slots:
    virtual void setMapName( const QString & s );
    virtual void setMapTitle( const QString & s );
    virtual void setComment( const QString & s );
    virtual void setAuthor( const QString & s );
    void setLockFile( bool b );

private:
    Ui::ExtraInfoDialog ui;

};

#endif // EXTRAINFODIALOG_H
