#include "heading.h"
#include "misc.h"

#include <QDebug>
#include <QRegExp>

/////////////////////////////////////////////////////////////////
// Heading
/////////////////////////////////////////////////////////////////

void Heading::operator=(const VymText &other) { copy(other); }

QString Heading::saveToDir()
{
    return valueElement("heading", VymText::saveToDir(), getAttributes());
}
