#ifndef DEBUG_H
#define DEBUG_H

#include <QDateTime>
#include <QString>

extern bool useActionLog;
extern QString actionLogPath;

void logDebug(const QString &comment,const QString &area = "",  const QString &func = "");

#endif
