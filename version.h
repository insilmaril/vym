#ifndef VERSION_H 
#define VERSION_H

#include <QString>

#define __VYM_NAME "VYM"
#define __VYM_VERSION "1.13.31"
//#define __VYM_CODENAME "Codename: RC-3"
#define __VYM_CODENAME "Codename: development version, not for production!"
#define __VYM_BUILD_DATE "2011-06-06"


bool checkVersion(const QString &);
bool checkVersion(const QString &, const QString &);

#endif
