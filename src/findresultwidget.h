#ifndef FINDRESULTWIDGET_H
#define FINDRESULTWIDGET_H

#include <QItemSelection>
#include <QWidget>

#include "findwidget.h"

class FindResultModel;
class TreeItem;
class VymModel;
class QTreeView;
class QPushButton;
class FindResultTreeView;
class FindWidget;

class FindResultWidget : public QWidget {
    Q_OBJECT

  public:
    FindResultWidget(QWidget *parent = nullptr);
    FindResultModel *getResultModel();
    void addItem(TreeItem *ti);
    void addItem(const QString &s);
    QString getFindText();

  public slots:
    void popup();
    void cancelPressed();
    void nextButtonPressed(QString, bool);
    void updateSelection(QItemSelection, QItemSelection);
    void setStatus(FindWidget::Status st);
    void searchFinished();

  signals:
    void hideFindResultWidget();
    void noteSelected(QString, int);
    void findPressed(QString, bool);

  public:
    FindWidget *findWidget;

  private:
    FindResultModel *resultsModel;
    FindResultTreeView *view;
};

#endif
