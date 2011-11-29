#include "treedelegate.h"

#include <QDebug>
#include <QPainter>

#include "treeitem.h"
#include "treemodel.h"

TreeDelegate::TreeDelegate(QObject *)
{
}

#include <QTextDocument>
#include "noteobj.h"
QString TreeDelegate::displayText (const QVariant & value, const QLocale & ) const  // FIXME-3 Ugly, but works...
{
    QString s=value.toString();
    if (Qt::mightBeRichText (s))
    {
	NoteObj no (s);
	s=no.getNoteASCII();
    }
    return s.trimmed().replace ('\n',' ').replace ('\t',' ');
}

