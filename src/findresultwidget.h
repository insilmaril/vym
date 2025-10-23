#ifndef FINDRESULTWIDGET_H
#define FINDRESULTWIDGET_H

#include <QItemSelection>
#include <QWidget>

#include "findcontrolswidget.h"

class FindResultModel;
class TreeItem;
class VymModel;
class QTreeView;
class QPushButton;
class FindResultTreeView;
class FindControlsWidget;

class FindResultWidget : public QWidget {
    Q_OBJECT

  public:
    FindResultWidget(QWidget *parent = nullptr);

  public slots:
    void switchFocus();

  public:
    void setFocus();
    FindResultModel *getResultModel();
    void addItem(TreeItem *ti);
    void addItem(const QString &s);
    QString getFindText();

  public slots:
    void popup();
    void cancelPressed();
    void closeWindow();
    void nextButtonPressed(QString, bool);
    void updateSelection(QItemSelection, QItemSelection);
    void setStatus(FindControlsWidget::Status st);
    void searchFinished();

  signals:
    void hideFindResultWidget();
    void noteSelected(QString, int);
    void findPressed(QString, bool);

  public:
    FindControlsWidget *findControlsWidget;

  private:
    FindResultModel *resultsModel;
    FindResultTreeView *view;
};

#endif
