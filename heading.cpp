#include "heading.h"
#include "misc.h"

#include <QRegExp>
#include <QDebug>

/////////////////////////////////////////////////////////////////
// Heading
/////////////////////////////////////////////////////////////////

void Heading::operator =(const VymText &other)
{
    copy (other);
}

QString Heading::saveToDir ()
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
    return beginElement ("heading"  + textModeAttr + fontHintAttr)
            + VymText::saveToDir()
            + endElement ("heading");
}

