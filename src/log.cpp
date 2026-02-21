#include "log.h"

#include "file.h"

extern bool useActionLog;
extern QString actionLogPath;

void logDebug(const QString &comment,const QString &area,  const QString &func)
{
    if (!useActionLog) return;

    QString f;
    if (!func.isEmpty())
        f = "::" + func;

    QString log = QString("\n// %1 [Debug %2] %3\n")
        .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs), f, comment);

    // std::cout << log.toStdString() << std::endl << std::flush;

    appendStringToFile(actionLogPath, log);
}
