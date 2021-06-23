#include "attributewidget.h"

AttributeWidget::AttributeWidget(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
    // ui.keyComboBox->setEditable (true);
    ui.valueComboBox->setEditable(true);
}

void AttributeWidget::setTable(AttributeTable *at) { table = at; }

void AttributeWidget::setKey(const QString &k)
{
    key = k;
    ui.keyComboBox->insertItem(ui.keyComboBox->count(), key);
}

void AttributeWidget::setValues(const QStringList &vl)
{
    ui.valueComboBox->clear();
    ui.valueComboBox->insertStringList(vl);
}

/*
void AttributeWidget::setValue (const QString &v)
{
}
*/

void AttributeWidget::keyTextChanged(const QString &t) {}

void AttributeWidget::valueTextChanged(const QString &t) {}
