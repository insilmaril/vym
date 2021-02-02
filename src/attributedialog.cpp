#include "attributedialog.h"

#include "attributewidget.h"

#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>

AttributeDialog::AttributeDialog(QWidget *parent) : QDialog(parent)
{
    if (this->objectName().isEmpty())
        this->setObjectName(QString::fromUtf8("AttributeDialog"));
    QSize size(468, 75);
    size = size.expandedTo(this->minimumSizeHint());
    this->resize(size);
    QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
    this->setSizePolicy(sizePolicy);

    vboxLayout = new QVBoxLayout(this);
    vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));

    tableLayout = new QVBoxLayout();
    tableLayout->setObjectName(QString::fromUtf8("tableLayout"));

    hboxLayout = new QHBoxLayout();
    hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
    addButton = new QPushButton(this);
    addButton->setObjectName(QString::fromUtf8("addButton"));

    hboxLayout->addWidget(addButton);

    spacerItem =
        new QSpacerItem(111, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    hboxLayout->addItem(spacerItem);

    closeButton = new QPushButton(this);
    closeButton->setObjectName(QString::fromUtf8("closeButton"));

    hboxLayout->addWidget(closeButton);

    vboxLayout->addLayout(tableLayout);
    vboxLayout->addLayout(hboxLayout);

    setWindowTitle(QApplication::translate("AttributeDialog", "Attributes", 0,
                                           QApplication::UnicodeUTF8));
    addButton->setText(QApplication::translate("AttributeDialog", "Add key", 0,
                                               QApplication::UnicodeUTF8));
    closeButton->setText(QApplication::translate("AttributeDialog", "Close", 0,
                                                 QApplication::UnicodeUTF8));

    connect(addButton, SIGNAL(clicked()), this, SLOT(addKey()));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(accept()));

    table = NULL;
}

void AttributeDialog::setTable(AttributeTable *t) { table = t; }

void AttributeDialog::setBranch(BranchObj *bo) { branch = bo; }

void AttributeDialog::setMode(const AttributeDialogMode &m)
{
    mode = m;

    QString title;
    if (mode == Definition)
        title = QApplication::translate("Attribute Dialog",
                                        "AttributeDialog - Edit definitions", 0,
                                        QApplication::UnicodeUTF8);
    else
        title = QApplication::translate("Attribute Dialog",
                                        "AttributeDialog - Edit %1", 0,
                                        QApplication::UnicodeUTF8)
                    .arg("objname");
    setWindowTitle(title);
}

void AttributeDialog::updateTable()
{
    if (table) {
        // Update list of keys and values
        QStringList keyList = table->getKeys();
        AttributeWidget *aw;
        for (int i = 0; i < keyList.count(); i++) {
            aw = new AttributeWidget(this);
            aw->setKey(keyList.at(i));
            // TODO aw->setValues (table->getValues (keyList.at(i) ));
            aw->show();
            tableLayout->addWidget(aw);
        }

        // Update attributes in dialog from data in selected branch

        // TODO
    }
}
void AttributeDialog::addKey()
{
    AttributeWidget *aw1 = new AttributeWidget(this);
    aw1->show();
    tableLayout->addWidget(aw1);
}

void AttributeDialog::closeEvent(QCloseEvent *ce)
{
    ce->accept(); // can be reopened with show()
    hide();
    emit(windowClosed());
    return;
}
