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
    return valueElement ("vymnote", VymText::saveToDir(), getAttributes() );
}

