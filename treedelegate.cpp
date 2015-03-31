#include "misc.h"
#include "treedelegate.h"

#include <QDebug>
#include <QPainter>

#include "treeitem.h"
#include "treemodel.h"

TreeDelegate::TreeDelegate(QObject *) // FIXME-000 shouldn't be necessary any longer...
{
}

#include <QTextDocument>
///  #include "vymnote.h"
QString TreeDelegate::displayText (const QVariant & value, const QLocale & ) const  // FIXME-4 Ugly, but works...
{
    return value.toString();

    QString s = value.toString();
    if (Qt::mightBeRichText (s)) s = richTextToPlain(s);
    return s.trimmed().replace ('\n',' ').replace ('\t',' ');
}

