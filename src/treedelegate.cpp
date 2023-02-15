#include "treedelegate.h"

TreeDelegate::TreeDelegate(QObject *) {}

QString TreeDelegate::displayText(const QVariant &value, const QLocale &) const
{
    return value.toString().trimmed();
}

void TreeDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
    const QModelIndex &index) const
{
    QStyleOptionViewItem local_option = option;
    /*  FIXME-0 not needed
    QColor color_base = local_option.features.testFlag(QStyleOptionViewItem::Alternate)
            ? local_option.palette.color(QPalette::AlternateBase)
            : local_option.palette.color(QPalette::Base);
            */
    QVariant color_text_v = index.model()->data(index, Qt::ForegroundRole);
    QColor color_text = color_text_v.isValid() ? color_text_v.value<QColor>()
                                               : local_option.palette.color(QPalette::Text);
    // clobber the selection coloring with ordinary coloring
    // local_option.palette.setColor(QPalette::Highlight, color_base);
    local_option.palette.setColor(QPalette::HighlightedText, color_text);
    QStyledItemDelegate::paint(painter, local_option, index) ;
}
