#include "vymnote.h"

/////////////////////////////////////////////////////////////////
// VymNote
/////////////////////////////////////////////////////////////////

void VymNote::operator= (const VymText &other)
{
    copy (other);
}

QString VymNote::saveToDir ()
{
    QString fontHintAttr;
    QString textModeAttr;
    if (textmode == RichText)
        textModeAttr = attribut("textMode","richText");
    else
    {
        textModeAttr = attribut("textMode","plainText");
        fontHintAttr  = attribut("fonthint",fonthint);
    }
    return beginElement ("vymnote"  + textModeAttr + fontHintAttr)
            + VymText::saveToDir()
            + endElement ("vymnote");
}

