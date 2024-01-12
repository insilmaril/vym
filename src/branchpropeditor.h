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
    void frameAutoDesignChanged();
    void frameTypeChanged(int i);
    void framePenColorClicked();
    void frameBrushColorClicked();
    void framePaddingChanged(int);
    void framePenWidthChanged(int);

    void branchesLayoutsChanged(int);
    void imagesLayoutsChanged(int);
    void childrenLayoutChanged();

    void linkHideUnselectedChanged(int);

    void rotationHeadingChanged(int);
    void rotationSubtreeChanged(int);

    void scaleHeadingChanged(qreal);
    void scaleHeadingSliderPressed();
    void scaleHeadingSliderChanged(int);
    void scaleHeadingSliderReleased();

    void scaleSubtreeChanged(qreal);
    void scaleSubtreeSliderPressed();
    void scaleSubtreeSliderChanged(int);
    void scaleSubtreeSliderReleased();

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

    qreal scaleHeadingInitialValue;
    qreal scaleSubtreeInitialValue;

    //AttributeDelegate attributeDelegate;
};

#endif //
