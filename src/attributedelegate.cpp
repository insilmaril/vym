#include <QtGui>

#include "attributedelegate.h"
#include <iostream>

using namespace ::std;

AttributeDelegate::AttributeDelegate(QObject *parent) : QItemDelegate(parent) {}

QWidget *
AttributeDelegate::createEditor(QWidget *parent,
                                const QStyleOptionViewItem & /* option */,
                                const QModelIndex &index) const
{
    int col = index.column();
    int row = index.row();

    if (col == 0 && row == index.model()->rowCount() - 1) {
        // We are editing a new attribute, starting with attribute name
        QComboBox *editor = new QComboBox(parent);
        editor->insertItems(0, attributeTable->getKeys());
        return editor;
    }
    if (col == 1 && row == index.model()->rowCount() - 1) {
        qDebug() << "Edit value now...";
        // We are editing a new attribute, starting with attribute name
        QComboBox *editor = new QComboBox(parent);
        editor->insertItems(0, attributeTable->getKeys());
        return editor;
    }

    // Is there already an atttribute defined or
    // do we need to create a new one?

    QVariant var =
        index.model()->data(index.model()->index(row, 2, QModelIndex()));
    QString typeName = var.toString();
    qDebug() << "AttrDel::createEditor type=" << qPrintable(typeName);

    if (typeName == "IntList") {
        QSpinBox *editor = new QSpinBox(parent);
        editor->setMinimum(0);
        editor->setMaximum(5);
        return editor;
    }
    else if (typeName == "FreeInt") {
        QSpinBox *editor = new QSpinBox(parent);
        editor->setMinimum(0);
        editor->setMaximum(100);
        return editor;
    }
    else if (typeName == "FreeString") {
        QComboBox *editor = new QComboBox(parent);
        return editor;
    }
    else if (typeName == "StringList") {
        QComboBox *editor = new QComboBox(parent);
        return editor;
    }

    return NULL;
}

void AttributeDelegate::setEditorData(QWidget *editor,
                                      const QModelIndex &index) const
{
    QVariant value = index.model()->data(index, Qt::DisplayRole);
    switch (value.type()) {
    case QVariant::Int: {
        int value = index.model()->data(index, Qt::DisplayRole).toInt();
        QSpinBox *spinBox = static_cast<QSpinBox *>(editor);
        spinBox->setValue(value);
        break;
    }
    /*
    {
        QString value = index.model()->data(index, Qt::DisplayRole).toString();
        QLineEdit *le= static_cast<QLineEdit*>(editor);
        le->setText(value);
        break;
    }
    */
    case QVariant::String: {
        QComboBox *cb = static_cast<QComboBox *>(editor);
        QStringList sl;
        sl << index.model()->data(index, Qt::DisplayRole).toString();
        cb->insertStringList(sl);
        break;
    }
    default:
        break;
    }
}

void AttributeDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                     const QModelIndex &index) const
{
    QVariant value = index.model()->data(index, Qt::DisplayRole);
    switch (value.type()) {
    case QVariant::Int: {
        QSpinBox *spinBox = static_cast<QSpinBox *>(editor);
        spinBox->interpretText();
        model->setData(index, spinBox->value(), Qt::EditRole);
        break;
    }
    case QVariant::String: {
        QComboBox *cb = static_cast<QComboBox *>(editor);
        model->setData(index, cb->currentText(), Qt::EditRole);
        break;
    }
    default:
        break;
    }
}

void AttributeDelegate::updateEditorGeometry(
    QWidget *editor, const QStyleOptionViewItem &option,
    const QModelIndex & /* index */) const
{
    editor->setGeometry(option.rect);
}

void AttributeDelegate::setAttributeTable(AttributeTable *table)
{
    attributeTable = table;
}
