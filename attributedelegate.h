
#ifndef ATTRIBUTEDELEGATE_H
#define ATTRIBUTEDELEGATE_H

#include <QItemDelegate>
#include <QModelIndex>
#include <QObject>
#include <QSize>
#include <QSpinBox>
#include <QLineEdit>

#include "attribute.h"

class AttributeDelegate : public QItemDelegate
{
    Q_OBJECT

enum EditorType {Undefined,SpinBox,LineEdit,ComboBox};

public:
    AttributeDelegate(QObject *parent = 0);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;

    void setEditorData(QWidget *editor, const QModelIndex &index) const ;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const ;

    void updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setAttributeTable(AttributeTable *table);
private:
    AttributeTable *attributeTable;
};

#endif
