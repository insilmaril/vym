#include "findresultwidget.h"

#include <QTreeView>
#include <QVBoxLayout>

#include "findresultitem.h"
#include "findresultmodel.h"
#include "vymmodel.h"

FindResultWidget::FindResultWidget(QWidget *)
{
    // Create results model
    resultsModel = new FindResultModel;

    // Create TreeView
    view = new QTreeView(this);
    view->setModel(resultsModel);

    // Create FindWidget
    findWidget = new FindWidget(this);
    connect(findWidget, SIGNAL(nextButtonPressed(QString, bool)), this,
            SLOT(nextButtonPressed(QString, bool)));

    QVBoxLayout *mainLayout = new QVBoxLayout;

    mainLayout->addWidget(view);
    mainLayout->addWidget(findWidget);

    setLayout(mainLayout);

    // Selection
    connect(view->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
            SLOT(updateSelection(QItemSelection, QItemSelection)));

    connect(resultsModel, SIGNAL(layoutChanged()), view, SLOT(expandAll()));
}

void FindResultWidget::addItem(TreeItem *ti)
{
    if (ti) {
        QModelIndex index = view->selectionModel()->currentIndex();
        // QAbstractItemModel *resultsModel = view->model();

        if (!resultsModel->insertRow(index.row() + 1, index.parent()))
            return;

        for (int column = 0; column < resultsModel->columnCount(index.parent());
             ++column) {
            QModelIndex child =
                resultsModel->index(index.row() + 1, column, index.parent());
            resultsModel->setData(child, QVariant(ti->headingPlain()),
                                  Qt::EditRole);
            resultsModel->getItem(child)->setOriginal(ti);
        }
    }
}

void FindResultWidget::addItem(const QString &s)
{
    if (!s.isEmpty()) {
        QModelIndex index = view->selectionModel()->currentIndex();

        if (!resultsModel->insertRow(index.row() + 1, index.parent()))
            return;

        for (int column = 0; column < resultsModel->columnCount(index.parent());
             ++column) {
            QModelIndex child =
                resultsModel->index(index.row() + 1, column, index.parent());
            resultsModel->setData(child, QVariant(s), Qt::EditRole);
        }
    }
}

QString FindResultWidget::getFindText() { return findWidget->getFindText(); }

FindResultModel *FindResultWidget::getResultModel() { return resultsModel; }

void FindResultWidget::popup()
{
    show();
    parentWidget()->show();
    findWidget->setFocus();
}

void FindResultWidget::cancelPressed() { emit(hideFindResultWidget()); }

void FindResultWidget::nextButtonPressed(QString s, bool searchNotesFlag)
{
    emit(findPressed(s, searchNotesFlag));
}

void FindResultWidget::updateSelection(QItemSelection newsel, QItemSelection)
{
    QModelIndex ix;
    foreach (ix, newsel.indexes()) {
        FindResultItem *fri =
            static_cast<FindResultItem *>(ix.internalPointer());
        if (fri->getOrgModel() && fri->getOriginalID() > 0) {
            TreeItem *ti = fri->getOrgModel()->findID(fri->getOriginalID());
            if (ti) {
                fri->getOrgModel()->select(ti);
                int i = fri->getOriginalIndex();
                if (i >= 0)
                    emit(noteSelected(resultsModel->getSearchString(), i));
            }
        }
    }
}

void FindResultWidget::setStatus(FindWidget::Status st)
{
    findWidget->setStatus(st);
}
