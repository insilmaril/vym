#ifndef MACROS_H
#define MACROS_H

#include <QString>

class Macros {
public:
    QString getPath ();
    QString getPath (const int &i);
    QString getMacro (const int &i);
};
#endif
