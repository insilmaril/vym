#include "findresultwidget.h"

#include <QTreeView>
#include <QVBoxLayout>

#include "findresultitem.h"
#include "findresultmodel.h"
#include "findresulttreeview.h"
#include "mainwindow.h"
#include "vymmodel.h"

extern Main *mainWindow;

FindResultWidget::FindResultWidget(QWidget *)
{
    // Create results model
    resultsModel = new FindResultModel;

    // Create TreeView
    view = new FindResultTreeView();
    view->setModel(resultsModel);

    // Create FindControlsWidget
    findControlsWidget = new FindControlsWidget(this);
    connect(findControlsWidget, SIGNAL(nextButtonPressed(QString, bool)), this,
            SLOT(nextButtonPressed(QString, bool)));

    QAction *a = new QAction("Cancel", findControlsWidget);
    a->setShortcut(Qt::Key_Escape);             // Escape in findControlsWidget
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(a, SIGNAL(triggered()), this, SLOT(cancelPressed()));
    addAction(a);

    a = new QAction("Close", findControlsWidget);
    a->setShortcut(Qt::CTRL | Qt::Key_D);       // Close window in findControlsWidget
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(a, SIGNAL(triggered()), this, SLOT(closeWindow()));
    addAction(a);

    a = new QAction("Switch focus", this);
    a->setShortcut(Qt::Key_Tab);                // Switch focus between QComboBox and QTreeView
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(a, SIGNAL(triggered()), this, SLOT(switchFocus()));
    addAction(a);

    QVBoxLayout *mainLayout = new QVBoxLayout;

    mainLayout->addWidget(view);
    mainLayout->addWidget(findControlsWidget);

    setLayout(mainLayout);

    // Selection
    connect(view->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
            SLOT(updateSelection(QItemSelection, QItemSelection)));

    connect(view, SIGNAL(searchFinished()), this, SLOT(searchFinished()));

    connect(resultsModel, SIGNAL(layoutChanged()), view, SLOT(expandAll()));
}

void FindResultWidget::switchFocus()
{
    if (view->hasFocus())
        setFocus();
    else
        view->setFocus();
}

void FindResultWidget::setFocus()
{
    findControlsWidget->setFocus();
}

void FindResultWidget::addItem(TreeItem *ti)
{
    if (ti) {
        QModelIndex index = view->selectionModel()->currentIndex();

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

QString FindResultWidget::getFindText() { return findControlsWidget->getFindText(); }

FindResultModel *FindResultWidget::getResultModel() { return resultsModel; }

void FindResultWidget::popup()
{
    show();
    parentWidget()->show();
    view->setFocus();
}

void FindResultWidget::cancelPressed()
{
    mainWindow->escapePressed();
}

void FindResultWidget::closeWindow()
{
    parentWidget()->hide();
}

void FindResultWidget::nextButtonPressed(QString s, bool searchNotesFlag)
{
    view->setFocus();
    emit findPressed(s, searchNotesFlag);
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
                    emit noteSelected(resultsModel->getSearchString(), i);
            }
        }
    }
}

void FindResultWidget::setStatus(FindControlsWidget::Status st)
{
    findControlsWidget->setStatus(st);
}

void FindResultWidget::searchFinished()
{
    QModelIndexList sl = view->selectionModel()->selectedIndexes();
    if (!sl.isEmpty() && sl.first().isValid()) {
        FindResultItem *fri =
            static_cast<FindResultItem *>(sl.first().internalPointer());
        if (fri->getOrgModel() && fri->getOriginalID() > 0) {
            TreeItem *ti = fri->getOrgModel()->findID(fri->getOriginalID());
            if (ti) {
                fri->getOrgModel()->select(ti);
                int i = fri->getOriginalIndex();
                if (i >= 0)
                    emit noteSelected(resultsModel->getSearchString(), i);
                parentWidget()->hide();
            }
        }
    }
}
