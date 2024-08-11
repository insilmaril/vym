#ifndef BRANCHPROPERTYEDITOR_H
#define BRANCHPROPERTYEDITOR_H

#include "ui_branchpropeditor.h"

#include <QCloseEvent>
#include <QDialog>
#include <QtGui>

// #include "attributedelegate.h"

class QAbstractItemModel;

class AttributeItem;
class BranchContainer;
class BranchItem;
class ImageItem;
class TreeItem;
class VymModel;

class BranchPropertyEditor : public QDialog {
    Q_OBJECT
  public:
    BranchPropertyEditor(QWidget *parent = 0);
    ~BranchPropertyEditor();

  public:
    void setItem(TreeItem *);
    void setModel(VymModel *);
    void updateControls();

  private:
    void updateLayoutControls();
    void updateDimensionControls();
    void updateRotationControls();
    void updateScalingControls();

  private slots:
    void colorChanged(QColor);

    void frameAutoDesignChanged();
    void frameTypeChanged(int i);
    void framePenColorClicked();
    void frameBrushColorClicked();
    void framePaddingChanged(int);
    void framePenWidthChanged(int);

    void branchesLayoutsChanged(int);
    void imagesLayoutsChanged(int);

    void linkHideUnselectedChanged(int);

    void columnWidthAutoChanged();
    void columnWidthChanged(int);

    void rotationsAutoChanged();
    void rotationHeadingChanged(int);
    void rotationSubtreeChanged(int);

    void scalingAutoChanged();
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
    void setSignalsBlocked(const bool);

    Ui::BranchPropertyEditor ui;

    QList<QWidget*> controlWidgets;
    bool signalsEnabled;

    BranchItem *branchItem;
    BranchContainer *branchContainer;
    ImageItem *imageItem;

    VymModel *model;

    int lastSelectedBranchTab;

    QStandardItemModel *attributeModel;
    AttributeItem *attributeItem;

    qreal scaleHeadingInitialValue;
    qreal scaleSubtreeInitialValue;

    //AttributeDelegate attributeDelegate;
};

#endif //
