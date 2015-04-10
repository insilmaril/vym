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
    return valueElement ("heading", VymText::saveToDir(), getAttributes() );
}

