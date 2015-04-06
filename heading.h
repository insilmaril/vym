#ifndef HEADING_H
#define HEADING_H

#include "vymtext.h"

/*! \brief The heading belonging to one OrnamentedObj */


class Heading:public VymText
{
public:
    void operator= (const VymText &other);
    QString saveToDir();
};
#endif
