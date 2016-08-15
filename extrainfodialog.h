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
    virtual void setStats( const QString & s );
    void setReadOnly( bool b );
    bool isReadOnly();

public slots:
    virtual void setMapName( const QString & s );
    virtual void setMapTitle( const QString & s );
    virtual void setComment( const QString & s );
    virtual void setAuthor( const QString & s );

private:
    Ui::ExtraInfoDialog ui;
    bool readOnly;

};

#endif // EXTRAINFODIALOG_H
