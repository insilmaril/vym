#ifndef NOTEOBJ_H
#define NOTEOBJ_H

#include <qstring.h>

class NoteObj;

#include "xmlobj.h"

/*! \brief The text note belonging to one OrnamentedObj */


class NoteObj:public XMLObj
{
public:
    enum TextMode {AutoText, PlainText, RichText};
    NoteObj();
    void operator= (const NoteObj &);
    void copy (NoteObj);
    void clear();
    void setText (const QString&);
    void setNoteRichText (const QString&);
    void setNotePlain (const QString&);
    QString getText() const;
    QString getNoteASCII();
    QString getNoteASCII(QString igdent, const int &width=0);
    QString getNoteOpenDoc();
    void setRichText(bool b);
    bool isRichText() const;
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
    TextMode textmode;
};
#endif
