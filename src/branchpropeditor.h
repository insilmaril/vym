#ifndef BRANCHPROPERTYEDITOR_H
#define BRANCHPROPERTYEDITOR_H

#include "ui_branchpropeditor.h"

#include <QCloseEvent>
#include <QDialog>
#include <QtGui>

// #include "attributedelegate.h"

class QAbstractItemModel;

class BranchItem;
class TreeItem;
class VymModel;

class BranchPropertyEditor : public QDialog {
    Q_OBJECT
  public:
    BranchPropertyEditor(QWidget *parent = 0);
    ~BranchPropertyEditor();
  private:
    void updateContainerLayoutButtons();
  public:
    void setItem(TreeItem *);
    void setModel(VymModel *);

  private slots:
    void frameTypeChanged(int i);
    void framePenColorClicked();
    void frameBrushColorClicked();
    void framePaddingChanged(int);
    void framePenWidthChanged(int);

    void linkHideUnselectedChanged(int);
    void childrenLayoutChanged();
    void rotationHeadingChanged(int);
    void rotationInnerContentChanged(int);
    void taskPriorityDeltaChanged(int);
    void addAttributeClicked();
    void deleteAttributeClicked();
    void indexChanged(int);

  signals:
    void windowClosed();

  protected:
    void closeEvent(QCloseEvent *);

  private:
    void connectSignals();
    void disconnectSignals();

    Ui::BranchPropertyEditor ui;

    BranchItem *branchItem;
    VymModel *model;

    int lastSelectedBranchTab;

    QStandardItemModel *attributeModel;

    //AttributeDelegate attributeDelegate;
};

#endif //
