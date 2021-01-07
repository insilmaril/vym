#include "vymnote.h"

/////////////////////////////////////////////////////////////////
// VymNote
/////////////////////////////////////////////////////////////////

VymNote::VymNote()
{
    clear();
}

VymNote::VymNote(const VymText &other)
{
    VymText::copy(other);
}

void VymNote::operator= (const VymText &other)
{
    copy (other);
}

QString VymNote::saveToDir ()
{
    return valueElement ("vymnote", VymText::saveToDir(), getAttributes() );
}

