#ifndef ATTRIBUTEWIDGET_H
#define ATTRIBUTEWIDGET_H

#include "ui_attributewidget.h"

#include "attribute.h"

#include <QWidget>

class AttributeWidget : public QWidget {
    Q_OBJECT
  public:
    AttributeWidget(QWidget *parent = 0);
    void setTable(AttributeTable *at = 0);
    void setKey(const QString &k);
    void setValues(const QStringList &vl);

  public slots:
    virtual void keyTextChanged(const QString &t);
    virtual void valueTextChanged(const QString &t);

  private:
    Ui::AttributeWidget ui;
    AttributeTable *table;
    QString key;
};
#endif
