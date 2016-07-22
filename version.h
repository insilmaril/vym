#ifndef VERSION_H 
#define VERSION_H

#include <QString>

#define __VYM_NAME "VYM" 
#define __VYM_VERSION "2.5.19"
//#define __VYM_CODENAME "Codename: Production release"
#define __VYM_CODENAME "Codename: development version, not for production!"
#define __VYM_BUILD_DATE "2016-07-22"
#define __VYM_HOME "http://www.insilmaril.de/vym"

bool versionLowerThanVym(const QString &);
bool versionLowerOrEqualThanVym(const QString &);
bool versionLowerOrEqual(const QString &, const QString &);

#endif
