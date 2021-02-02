#ifndef TREEDELEGATE_H
#define TREEDELEGATE_H

#include <QStyledItemDelegate>

class TreeDelegate : public QStyledItemDelegate {
    Q_OBJECT
  public:
    TreeDelegate(QObject *parent = 0);
    QString displayText(const QVariant &value, const QLocale &locale) const;
    /*
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
        const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option,
        const QModelIndex &index ) const;
    */
};

#endif
