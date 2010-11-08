#include "treedelegate.h"

#include <QDebug>
#include <QPainter>

#include "treeitem.h"
#include "treemodel.h"

TreeDelegate::TreeDelegate(QObject *)
{
}

/*
*/
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

/*
void TreeDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);	//FIXME use own pant method to convert richtext 2 plaintext
    return;
    painter->save();

    QRectF rect=option.rect;
    QString text; //= qVariantValue <QString> (index.data());

    TreeItem *ti=((TreeModel*)index.model())->getItem(index);

    text=ti->getHeadingPlain();

    // Paint highlighted, if selected	    //FIXME not as nice as original implementation with bright borders and gradient
     if (option.state & QStyle::State_Selected)
	painter->fillRect(option.rect, option.palette.highlight() );


    painter->drawText(rect, Qt::AlignLeft, text );  

    painter->restore();

}
QSize TreeDelegate::sizeHint(const QStyleOptionViewItem &option,
                    const QModelIndex &index ) const
{
    return QStyledItemDelegate::sizeHint(option, index);    //FIXME sizeHint incorrect for RichText
}
*/
