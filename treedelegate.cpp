#include "treedelegate.h"

TreeDelegate::TreeDelegate(QObject *) {}

///  #include "vymnote.h"
QString TreeDelegate::displayText(const QVariant &value, const QLocale &) const
{
    return value.toString().trimmed();
}
