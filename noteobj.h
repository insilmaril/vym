#ifndef NOTEOBJ_H
#define NOTEOBJ_H

#include <qstring.h>

class NoteObj;

#include "xmlobj.h"

/*! \brief The text note belonging to one OrnamentedObj */


class NoteObj:public XMLObj
{
public:
    NoteObj();
    NoteObj(const QString&);
    void operator= (const NoteObj &);
    void copy (NoteObj);
    void clear();
    void setNote (const QString&);
    void setNoteMasked (const QString&);
    QString getNote() const;
    QString getNoteASCII();
    QString getNoteASCII(QString igdent, const int &width=0);
    QString getNoteOpenDoc();
    bool isRichText();
    void setFontHint (const QString&);
    QString getFontHint () const;
    void setFilenameHint (const QString&);
    QString getFilenameHint () const;
    bool isEmpty();
    QString saveToDir();

private:
    QString note;
    QString fonthint;
    QString filenamehint;
};
#endif
