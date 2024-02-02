#include "branchpropeditor.h"

#include <QColorDialog>

#include "attributeitem.h"
#include "branch-container.h"
#include "branchitem.h"
#include "settings.h"
#include "vymmodel.h"

extern Settings settings;
extern QString vymName;

BranchPropertyEditor::BranchPropertyEditor(QWidget *parent)
    : QDialog(parent) // FIXME-4 not updating when data is set elsewhere
                      // (connect to dataCHanged)

{
    ui.setupUi(this);

    setWindowTitle(vymName + " - " + tr("Property Editor", "Window caption"));

    branchItem = nullptr;
    branchContainer = nullptr;
    model = nullptr;

    ui.tabWidget->setEnabled(false);

    QPixmap pix(16, 16);
    pix.fill(QColor(Qt::black));
    ui.innerFramePenColorButton->setIcon(pix);
    ui.innerFrameBrushColorButton->setIcon(pix);

    // Remember the last tab, which was used when a branch was selected
    lastSelectedBranchTab = -1;

    if (!settings.value("/mainwindow/showTestMenu", false).toBool())
        ui.tabWidget->widget(3)->hide();

    //Create Model and View to hold attributes
    attributeModel = new QStandardItemModel (1, 3, this);   // FIXME-4 used and needed?
    attributeModel->setHeaderData(0,
            Qt::Horizontal,
            tr("Name","Branchprop window: Attribute name"));
    attributeModel->setHeaderData(1,
            Qt::Horizontal,
            tr("Value","Branchprop window: Attribute value"));
    attributeModel->setHeaderData(2,
            Qt::Horizontal,
            tr("Type","Branchprop window: Attribute type"));
    ui.attributeTableView->setModel (attributeModel);

    // Load Settings
    resize(
        settings
            .value("/satellite/propertywindow/geometry/size", QSize(450, 600))
            .toSize());
    move(settings
             .value("/satellite/propertywindow/geometry/pos", QPoint(250, 50))
             .toPoint());

    ui.tabWidget->setCurrentIndex(
        settings.value("/satellite/propertywindow/currentIndex", 0).toInt());

    if (settings.value("/satellite/propertywindow/showWithMain", true).toBool())
        show();
    else
        hide();

    scaleHeadingInitialValue = 1;
    scaleSubtreeInitialValue = 1;

    connect(ui.tabWidget, SIGNAL(currentChanged(int)), this,
            SLOT(indexChanged(int)));

    connectSignals();

    // Build list of controlWidgets, which later will get
    // signals blocked during changes not triggered by user
    signalsEnabled = true;
    QList <QWidget*> children = this->findChildren<QWidget *>();
    QMutableListIterator <QWidget*> it(children);

    // Only controlWidgets contain names below:
    QStringList controls;
    controls <<
        "Button" <<
        "Combo" <<
        "LineEdit" <<
        "SpinBox" <<
        "Slider" <<
        "CheckBox";
    while (it.hasNext()) {
        QWidget *w = it.next();
        foreach (auto s, controls)
            if (w->objectName().contains(s)) {
                controlWidgets << w;
                break;
            }
    }
}

BranchPropertyEditor::~BranchPropertyEditor()
{
    settings.setValue("/satellite/propertywindow/geometry/size", size());
    settings.setValue("/satellite/propertywindow/geometry/pos", pos());
    settings.setValue("/satellite/propertywindow/showWithMain", isVisible());
    settings.setValue("/satellite/propertywindow/currentIndex", ui.tabWidget->currentIndex());

    attributeModel->clear();
    delete (attributeModel);
}

void BranchPropertyEditor::setItem(TreeItem *ti)
{
    // Reset pointers to currently displayed TreeItem
    branchItem = nullptr;
    branchContainer = nullptr;
    imageItem = nullptr;

    if (!ti)
        ui.tabWidget->setEnabled(false);
    else if (ti->hasTypeBranch()) {
        branchItem = (BranchItem *)ti;
        branchContainer = branchItem->getBranchContainer();

        if (lastSelectedBranchTab >= 0)
            ui.tabWidget->setCurrentIndex(lastSelectedBranchTab);

        lastSelectedBranchTab = ui.tabWidget->currentIndex();

        // Activate the right tabs for this branch
        ui.tabWidget->setEnabled(true);
        for (int i = 0; i < 4; ++i)
            ui.tabWidget->setTabEnabled(i, true);
        ui.tabWidget->setTabEnabled(4, false);
    } else if (ti->hasTypeImage())
        imageItem = (ImageItem*)ti;

    updateControls();
}

void BranchPropertyEditor::updateControls()
{
    setSignalsEnabled(false);

    if (branchItem) {
        // Inner frame
        ui.innerFrameAutoDesignCheckBox->setChecked(branchContainer->frameAutoDesign(true));
        FrameContainer::FrameType t = branchContainer->frameType(true);
        ui.innerFrameTypeCombo->setEnabled(!branchContainer->frameAutoDesign(true));

        bool b = (t != FrameContainer::NoFrame && !branchContainer->frameAutoDesign(true));
        ui.innerFrameTypeLabel->setEnabled(b);
        ui.innerFramePenColorButton->setEnabled(b);
        ui.innerFrameBrushColorButton->setEnabled(b);
        ui.innerFramePaddingSpinBox->setEnabled(b);
        ui.innerFrameWidthSpinBox->setEnabled(b);
        ui.innerFramePaddingLabel->setEnabled(b);
        ui.innerFrameBorderLabel->setEnabled(b);
        ui.innerFrameBrushColorLabelDesc->setEnabled(b);
        ui.innerFramePenColorLabelDesc->setEnabled(b);

        QPixmap pix(16, 16);
        pix.fill(branchContainer->framePenColor(true));
        ui.innerFramePenColorButton->setIcon(pix);
        pix.fill(branchContainer->frameBrushColor(true));
        ui.innerFrameBrushColorButton->setIcon(pix);
        ui.innerFramePaddingSpinBox->setValue(branchContainer->framePadding(true));
        ui.innerFrameWidthSpinBox->setValue( branchContainer->framePenWidth(true));

        switch (t) {
            case FrameContainer::NoFrame:
                ui.innerFrameTypeCombo->setCurrentIndex(0);
                break;
            case FrameContainer::Rectangle:
                ui.innerFrameTypeCombo->setCurrentIndex(1);
                break;
            case FrameContainer::RoundedRectangle:
                ui.innerFrameTypeCombo->setCurrentIndex(2);
                break;
            case FrameContainer::Ellipse:
                ui.innerFrameTypeCombo->setCurrentIndex(3);
                break;
            case FrameContainer::Circle:
                ui.innerFrameTypeCombo->setCurrentIndex(4);
                break;
            case FrameContainer::Cloud:
                ui.innerFrameTypeCombo->setCurrentIndex(5);
                break;
            default:
                break;
        }

        // Outer frame
        ui.outerFrameAutoDesignCheckBox->setChecked(branchContainer->frameAutoDesign(false));
        t = branchContainer->frameType(false);
        ui.outerFrameTypeCombo->setEnabled(!branchContainer->frameAutoDesign(false));

        b = (t != FrameContainer::NoFrame && !branchContainer->frameAutoDesign(false));
        ui.outerFrameTypeLabel->setEnabled(b);
        ui.outerFramePenColorButton->setEnabled(b);
        ui.outerFrameBrushColorButton->setEnabled(b);
        ui.outerFramePaddingSpinBox->setEnabled(b);
        ui.outerFrameWidthSpinBox->setEnabled(b);
        ui.outerFramePaddingLabel->setEnabled(b);
        ui.outerFrameBorderLabel->setEnabled(b);
        ui.outerFrameBrushColorLabelDesc->setEnabled(b);
        ui.outerFramePenColorLabelDesc->setEnabled(b);

        pix.fill(branchContainer->framePenColor(false));
        ui.outerFramePenColorButton->setIcon(pix);
        pix.fill(branchContainer->frameBrushColor(false));
        ui.outerFrameBrushColorButton->setIcon(pix);
        ui.outerFramePaddingSpinBox->setValue(branchContainer->framePadding(false));
        ui.outerFrameWidthSpinBox->setValue( branchContainer->framePenWidth(false));

        switch (t) {
            case FrameContainer::NoFrame:
                ui.outerFrameTypeCombo->setCurrentIndex(0);
                break;
            case FrameContainer::Rectangle:
                ui.outerFrameTypeCombo->setCurrentIndex(1);
                break;
            case FrameContainer::RoundedRectangle:
                ui.outerFrameTypeCombo->setCurrentIndex(2);
                break;
            case FrameContainer::Ellipse:
                ui.outerFrameTypeCombo->setCurrentIndex(3);
                break;
            case FrameContainer::Circle:
                ui.outerFrameTypeCombo->setCurrentIndex(4);
                break;
            case FrameContainer::Cloud:
                ui.outerFrameTypeCombo->setCurrentIndex(5);
                break;
            default:
                break;
        }

        // Layout branches


        updateLayoutControls();
        updateRotationControls();
        updateScalingControls();

        // Link
        if (branchItem->getHideLinkUnselected())
            ui.hideLinkIfUnselectedCheckBox->setCheckState(Qt::Checked);
        else
            ui.hideLinkIfUnselectedCheckBox->setCheckState(Qt::Unchecked);

        // Task
        Task *task = branchItem->getTask();
        if (task) {
            ui.taskPrioDeltaSpinBox->setEnabled(true);
            ui.taskPrioDeltaSpinBox->setValue(task->getPriorityDelta());
            ui.dateCreationLineEdit->setText(
                task->getDateCreation().toString() + " - " +
                QString(tr("%1 days ago", "task related times"))
                    .arg(task->getAgeCreation()));
            QDateTime dt = task->getDateModification();
            if (dt.isValid()) {
                ui.dateModificationLineEdit->setText(
                    dt.toString() + " - " +
                    QString(tr("%1 days ago", "task related times"))
                        .arg(task->getAgeModification()));
            }
            else {
                ui.dateModificationLineEdit->setText("");
            }

            dt = task->getSleep();
            if (dt.isValid()) {
                QString s;
                qint64 daysSleep = task->getDaysSleep();
                daysSleep >= 0 ? s = QString(dt.toString() + " - " +
                                             tr("sleeping %1 days",
                                                "task related times"))
                                         .arg(daysSleep)
                               : s = QString(tr("Task is awake",
                                                "task related times"));
                ui.sleepLineEdit->setText(s);
            }
            else {
                ui.sleepLineEdit->setText("");
            }
        }
        else {
            ui.taskPrioDeltaSpinBox->setEnabled(false);
            ui.taskPrioDeltaSpinBox->setValue(0);
            ui.dateCreationLineEdit->setText("");
            ui.dateModificationLineEdit->setText("");
            ui.sleepLineEdit->setText("");
        }

    // Attributes
    attributeModel->removeRows(0, attributeModel->rowCount(), QModelIndex());

    for (int i = 0; i < branchItem->attributeCount(); i++)
    {
        AttributeItem *ai = branchItem->getAttributeNum(i);
        if (ai) {
            attributeModel->insertRow (i, QModelIndex ());
            attributeModel->setData(attributeModel->index(i, 0, QModelIndex()),
                ai->getKey());
            attributeModel->setData(attributeModel->index(i, 1, QModelIndex()),
                ai->getValue().toString());
            attributeModel->setData(attributeModel->index(i, 2, QModelIndex()),
                ai->getAttributeTypeString());
        }
    }

    ui.attributeTableView->resizeColumnsToContents();

    // Initialize Delegate  // FIXME-4 still needed?
    //attributeDelegate.setAttributeTable (mapEditor->attributeTable());
    //ui.attributeTableView->setItemDelegate (&attributeDelegate);

    } // BranchItem
    else if (imageItem) {
        ui.tabWidget->setEnabled(true);
        for (int i = 0; i < ui.tabWidget->count(); ++i)
            ui.tabWidget->setTabEnabled(i, false);
        ui.tabWidget->setTabEnabled(3, true);
        ui.tabWidget->setCurrentIndex(3);
    } else if (attributeItem) {
        ui.tabWidget->setEnabled(true);
        for (int i = 0; i < ui.tabWidget->count(); ++i)
            ui.tabWidget->setTabEnabled(i, false);
        ui.tabWidget->setTabEnabled(4, true);
    } else {
        ui.tabWidget->setEnabled(false);
    }
    setSignalsEnabled(true);
}

void BranchPropertyEditor::setModel(VymModel *m)
{
    model = m;
    if (model) {
        QList <TreeItem*> seltis = model->getSelectedItems();
        if (!seltis.isEmpty()) {
            setItem(seltis.first());
            return;
        }
    }
    ui.tabWidget->setEnabled(false);
}

void BranchPropertyEditor::updateLayoutControls()
{
    if (!branchContainer)
        return;

    // Layout branches
    if (branchContainer->branchesContainerAutoLayout)
        ui.branchesLayoutsCombo->setCurrentIndex(0);
    else {
        switch (branchContainer->getBranchesContainerLayout()) {
            case Container::Vertical:
                ui.branchesLayoutsCombo->setCurrentIndex(1);
                break;
            case Container::Horizontal:
                ui.branchesLayoutsCombo->setCurrentIndex(2);
                break;
            case Container::FloatingBounded:
                ui.branchesLayoutsCombo->setCurrentIndex(3);
                break;
            case Container::FloatingFree:
                ui.branchesLayoutsCombo->setCurrentIndex(4);
                break;
            case Container::List:
                ui.branchesLayoutsCombo->setCurrentIndex(5);
                break;
            default:
                qWarning() << QString("BranchPropEditor: Unknown branches layout '%1'").arg(branchContainer->getLayoutString(branchContainer->getBranchesContainerLayout()));
                qDebug() << "branch=" << branchItem->getHeadingPlain();
        }
    }

    // Layout images
    if (branchContainer->imagesContainerAutoLayout)
        ui.imagesLayoutsCombo->setCurrentIndex(0);
    else {
        switch (branchContainer->getImagesContainerLayout()) {
            case Container::Vertical:
                ui.imagesLayoutsCombo->setCurrentIndex(1);
                break;
            case Container::Horizontal:
                ui.imagesLayoutsCombo->setCurrentIndex(2);
                break;
            case Container::FloatingBounded:
                ui.imagesLayoutsCombo->setCurrentIndex(3);
                break;
            case Container::FloatingFree:
                ui.imagesLayoutsCombo->setCurrentIndex(4);
                break;
            default:
                qWarning() << QString("BranchPropEditor: Unknown images layout '%1'").arg(branchContainer->getLayoutString(branchContainer->getImagesContainerLayout()));
                qDebug() << "branch=" << branchItem->getHeadingPlain();
        }
    }
}

void BranchPropertyEditor::updateRotationControls()
{
    bool b;
    if (branchContainer) {
        b = branchContainer->rotationsAutoDesign();
        ui.rotationsAutoCheckBox->setEnabled(true);
        ui.rotationHeadingSlider->setValue(branchContainer->rotationHeading());
        ui.rotationSubtreeSlider->setValue(branchContainer->rotationSubtree());
        ui.rotationHeadingSpinBox->setValue(branchContainer->rotationHeading());
        ui.rotationSubtreeSpinBox->setValue(branchContainer->rotationSubtree());
    } else {
        b = true;
        ui.rotationsAutoCheckBox->setEnabled(false);
    }

    ui.rotationHeadingSlider->setEnabled(!b);
    ui.rotationSubtreeSlider->setEnabled(!b);
    ui.rotationHeadingSpinBox->setEnabled(!b);
    ui.rotationSubtreeSpinBox->setEnabled(!b);
    ui.rotationsAutoCheckBox->setChecked(b);
}

void BranchPropertyEditor::updateScalingControls()
{
    bool b;
    if (branchContainer) {
        b = branchContainer->scalingAutoDesign();
        ui.scalingAutoCheckBox->setEnabled(true);
        ui.scaleHeadingSpinBox->setValue(branchContainer->scaleHeading());
        ui.scaleSubtreeSpinBox->setValue(branchContainer->scaleSubtree());

    } else {
        b = true;
        ui.scalingAutoCheckBox->setEnabled(false);
    }

    ui.scaleHeadingSlider->setEnabled(!b);
    ui.scaleSubtreeSlider->setEnabled(!b);
    ui.scaleHeadingSpinBox->setEnabled(!b);
    ui.scaleSubtreeSpinBox->setEnabled(!b);
    ui.scalingAutoCheckBox->setChecked(b);
}

void BranchPropertyEditor::frameAutoDesignChanged()
{
    if (model) {
        if ((sender() == ui.innerFrameAutoDesignCheckBox))
            model->setFrameAutoDesign(true, ui.innerFrameAutoDesignCheckBox->isChecked());
        else
            model->setFrameAutoDesign(false, ui.outerFrameAutoDesignCheckBox->isChecked());
        updateControls();
    }
}

void BranchPropertyEditor::frameTypeChanged(int i)
{
    if (model) {
        bool useInnerFrame = (sender() == ui.innerFrameTypeCombo) ? true : false;
        switch (i) {
            case 0:
                model->setFrameType(useInnerFrame, FrameContainer::NoFrame);
                break;
            case 1:
                model->setFrameType(useInnerFrame, FrameContainer::Rectangle);
                break;
            case 2:
                model->setFrameType(useInnerFrame, FrameContainer::RoundedRectangle);
                break;
            case 3:
                model->setFrameType(useInnerFrame, FrameContainer::Ellipse);
                break;
            case 4:
                model->setFrameType(useInnerFrame, FrameContainer::Circle);
                break;
            case 5:
                model->setFrameType(useInnerFrame, FrameContainer::Cloud);
                break;
        }

        // Update data in dialog
        updateControls();
    }
}

void BranchPropertyEditor::framePenColorClicked()
{
    bool useInnerFrame = (sender() == ui.innerFramePenColorButton) ? true : false;

    if (model) {
        QColor col = Qt::white;
        if (branchItem) {
            BranchContainer *bc = branchItem->getBranchContainer();
            if (bc->frameType(useInnerFrame) != FrameContainer::NoFrame)
                col = bc->framePenColor(useInnerFrame);

            col = QColorDialog::getColor(col, this);
            if (col.isValid()) {
                model->setFramePenColor(useInnerFrame, col);

                // update color button
                updateControls();
            }
        }
    }
}

void BranchPropertyEditor::frameBrushColorClicked()
{
    if (model) {
        bool useInnerFrame = (sender() == ui.innerFrameBrushColorButton) ? true : false;
        QColor col = Qt::white;
        if (branchItem) {
            BranchContainer *bc = branchItem->getBranchContainer();
            if (bc->frameType(useInnerFrame) != FrameContainer::NoFrame)
                col = bc->frameBrushColor(useInnerFrame);

            col = QColorDialog::getColor(
                    col,
                    this,
                    tr("Background color of frame","Branch property dialog"),
                    QColorDialog::ShowAlphaChannel);
            if (col.isValid()) {
                model->setFrameBrushColor(useInnerFrame, col);

                // update color button
                updateControls();
            }
        }
    }
}

void BranchPropertyEditor::framePaddingChanged(int i)
{
    if (model) {
        bool useInnerFrame = (sender() == ui.innerFramePaddingSpinBox) ? true : false;
        model->setFramePadding(useInnerFrame, i);
    }
}

void BranchPropertyEditor::framePenWidthChanged(int i)
{
    if (model) {
        bool useInnerFrame = (sender() == ui.innerFrameWidthSpinBox) ? true : false;
        model->setFramePenWidth(useInnerFrame, i);
    }
}

void BranchPropertyEditor::linkHideUnselectedChanged(int i)
{
    model->setHideLinkUnselected(i);
}

void BranchPropertyEditor::branchesLayoutsChanged(int i)
{
    if (!model) return;

    QString s;
    switch (i) {
        case 0:
            s = "Auto";
            break;
        case 1:
            s = "Vertical";
            break;
        case 2:
            s = "Horizontal";
            break;
        case 3:
            s = "FloatingBounded";
            break;
        case 4:
            s = "FloatingFree";
            break;
        case 5:
            s = "List";
            break;
        default:
            qWarning() << "BranchPropertyEditor unknown layout in line " << __LINE__;
            return;
    }

    model->setBranchesLayout(s);
}

void BranchPropertyEditor::imagesLayoutsChanged(int i)
{
    if (!model) return;

    QString s;
    switch (i) {
        case 0:
            s = "Auto";
            break;
        case 1:
            s = "Vertical";
            break;
        case 2:
            s = "Horizontal";
            break;
        case 3:
            s = "Floating bounded";
            break;
        case 4:
            s = "Floating free";
            break;
        default:
            qWarning() << "BranchPropertyEditor unknown layout in line " << __LINE__;
            return;
    }

    model->setImagesLayout(s);
}

void BranchPropertyEditor::rotationsAutoChanged()
{
    if (model) {
        model->setRotationsAutoDesign(ui.rotationsAutoCheckBox->isChecked());
        updateControls();
    }
}

void BranchPropertyEditor::rotationHeadingChanged(int i)    // FIXME-4 Create custom class to sync slider and spinbox and avoid double calls to models
{
    if (model)
        model->setRotationHeading(i);

    ui.rotationHeadingSlider->setValue(i);
    ui.rotationHeadingSpinBox->setValue(i);
}

void BranchPropertyEditor::rotationSubtreeChanged(int i)
{
    if (model)
        model->setRotationSubtree(i);

    ui.rotationSubtreeSlider->setValue(i);
    ui.rotationSubtreeSpinBox->setValue(i);
}

void BranchPropertyEditor::scalingAutoChanged()
{
    if (model) {
        model->setScalingAutoDesign(ui.scalingAutoCheckBox->isChecked());
    }
}

void BranchPropertyEditor::scaleHeadingChanged(qreal f)    // FIXME-5 Create custom class to sync slider and spinbox and avoid double calls to models
{
    if (model)
        model->setScaleHeading(f);
}

void BranchPropertyEditor::scaleHeadingSliderPressed()
{
    if (model)
        scaleHeadingInitialValue = model->getScaleHeading();
}

void BranchPropertyEditor::scaleHeadingSliderChanged(int i)
{
    qreal v = (qreal) i / 100 + scaleHeadingInitialValue;
    if (model)
        model->setScaleHeading(v);
}

void BranchPropertyEditor::scaleHeadingSliderReleased()
{
    ui.scaleHeadingSlider->blockSignals(true);
    ui.scaleHeadingSlider->setValue(0);
    ui.scaleHeadingSlider->blockSignals(false);
}

void BranchPropertyEditor::scaleSubtreeChanged(qreal f)
{
    if (model)
        model->setScaleSubtree(f);
}

void BranchPropertyEditor::scaleSubtreeSliderPressed()
{
    if (model)
        scaleSubtreeInitialValue = model->getScaleSubtree();
}

void BranchPropertyEditor::scaleSubtreeSliderChanged(int i)
{
    qreal v = (qreal) i / 100 + scaleSubtreeInitialValue;
    if (model)
        model->setScaleSubtree(v);
}

void BranchPropertyEditor::scaleSubtreeSliderReleased()
{
    ui.scaleSubtreeSlider->blockSignals(true);
    ui.scaleSubtreeSlider->setValue(0);
    ui.scaleSubtreeSlider->blockSignals(false);
}

void BranchPropertyEditor::taskPriorityDeltaChanged(int n)
{
    if (model)
        model->setTaskPriorityDelta(n);
}

void BranchPropertyEditor::closeEvent(QCloseEvent *ce)
{
    ce->accept(); // can be reopened with show()
    hide();
    emit(windowClosed());
    return;
}

void BranchPropertyEditor::addAttributeClicked()    // FIXME-4 not used currently
{
    qDebug() << "BranchPropEditor::addAttribute";

/*
    // Add empty line for adding attributes
    attributeModel->insertRow (attributeModel->rowCount (),QModelIndex ());
    attributeModel->setData(attributeModel->index(attributeModel->rowCount()-1,
0, QModelIndex()),  "Add new");
    attributeModel->setData(attributeModel->index(attributeModel->rowCount()-1,
2, QModelIndex()),  "Undefined");

    // Select attribute from list
    ui.attributeTableView->edit
(attributeModel->index(attributeModel->rowCount()-1,0, QModelIndex() ));
    ui.attributeTableView->resizeColumnsToContents();

//  QString attname=attributeModel->in
//  attributeModel->setData(attributeModel->index(attributeModel->rowCount()-1,
2, QModelIndex()),  );



    ui.attributeTableView->edit
(attributeModel->index(attributeModel->rowCount()-1,1, QModelIndex() ));
*/

}

void BranchPropertyEditor::deleteAttributeClicked()
{
    qDebug() << "BranchPropEditor::deleteAttribute";
}

void BranchPropertyEditor::indexChanged(int n)
{
    lastSelectedBranchTab = n;
}

void BranchPropertyEditor::connectSignals()
{
    // Frames
    connect(ui.innerFrameAutoDesignCheckBox, SIGNAL(clicked()), this,
            SLOT(frameAutoDesignChanged()));
    connect(ui.innerFramePenColorButton, SIGNAL(clicked()), this,
            SLOT(framePenColorClicked()));
    connect(ui.innerFramePaddingSpinBox, SIGNAL(valueChanged(int)), this,
            SLOT(framePaddingChanged(int)));
    connect(ui.innerFrameWidthSpinBox, SIGNAL(valueChanged(int)), this,
            SLOT(framePenWidthChanged(int)));
    connect(ui.innerFrameBrushColorButton, SIGNAL(clicked()), this,
            SLOT(frameBrushColorClicked()));
    connect(ui.innerFrameTypeCombo, SIGNAL(currentIndexChanged(int)), this,
            SLOT(frameTypeChanged(int)));

    connect(ui.outerFrameAutoDesignCheckBox, SIGNAL(clicked()), this,
            SLOT(frameAutoDesignChanged()));
    connect(ui.outerFramePenColorButton, SIGNAL(clicked()), this,
            SLOT(framePenColorClicked()));
    connect(ui.outerFramePaddingSpinBox, SIGNAL(valueChanged(int)), this,
            SLOT(framePaddingChanged(int)));
    connect(ui.outerFrameWidthSpinBox, SIGNAL(valueChanged(int)), this,
            SLOT(framePenWidthChanged(int)));
    connect(ui.outerFrameBrushColorButton, SIGNAL(clicked()), this,
            SLOT(frameBrushColorClicked()));

    connect(ui.outerFrameTypeCombo, SIGNAL(currentIndexChanged(int)), this,
            SLOT(frameTypeChanged(int)));

    // Layout
    connect(ui.branchesLayoutsCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(branchesLayoutsChanged(int)));
    connect(ui.imagesLayoutsCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(imagesLayoutsChanged(int)));

    connect(ui.rotationsAutoCheckBox, SIGNAL(clicked()),
            this, SLOT(rotationsAutoChanged()));
    connect(ui.rotationHeadingSlider, SIGNAL(valueChanged(int)),
            this, SLOT(rotationHeadingChanged(int)));
    connect(ui.rotationHeadingSpinBox, SIGNAL(valueChanged(int)),
            this, SLOT(rotationHeadingChanged(int)));

    // With lambda          // FIXME-3
    // connect(spinbox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), slider, &QSlider::setValue);

    connect(ui.rotationSubtreeSlider, SIGNAL(valueChanged(int)),
            this, SLOT(rotationSubtreeChanged(int)));
    connect(ui.rotationSubtreeSpinBox, SIGNAL(valueChanged(int)),
            this, SLOT(rotationSubtreeChanged(int)));

    connect(ui.scalingAutoCheckBox, SIGNAL(clicked()),
            this, SLOT(scalingAutoChanged()));
    connect(ui.scaleHeadingSpinBox, SIGNAL(valueChanged(qreal)),
            this, SLOT(scaleHeadingChanged(qreal)));
    connect(ui.scaleHeadingSlider, SIGNAL(sliderPressed()),
            this, SLOT(scaleHeadingSliderPressed()));
    connect(ui.scaleHeadingSlider, SIGNAL(valueChanged(int)),
            this, SLOT(scaleHeadingSliderChanged(int)));
    connect(ui.scaleHeadingSlider, SIGNAL(sliderReleased()),
            this, SLOT(scaleHeadingSliderReleased()));

    connect(ui.scaleSubtreeSpinBox, SIGNAL(valueChanged(qreal)),
            this, SLOT(scaleSubtreeChanged(qreal)));
    connect(ui.scaleSubtreeSlider, SIGNAL(sliderPressed()),
            this, SLOT(scaleSubtreeSliderPressed()));
    connect(ui.scaleSubtreeSlider, SIGNAL(valueChanged(int)),
            this, SLOT(scaleSubtreeSliderChanged(int)));
    connect(ui.scaleSubtreeSlider, SIGNAL(sliderReleased()),
            this, SLOT(scaleSubtreeSliderReleased()));

    // Link
    connect(ui.hideLinkIfUnselectedCheckBox, SIGNAL(stateChanged(int)), this,
            SLOT(linkHideUnselectedChanged(int)));

    // Tasks
    connect(ui.taskPrioDeltaSpinBox, SIGNAL(valueChanged(int)), this,
            SLOT(taskPriorityDeltaChanged(int)));

    // Attributes
    // For the time being hide above buttons, not used
    /*
    connect (
        ui.addAttributeButton, SIGNAL (clicked()),
        this, SLOT (addAttributeClicked()));
    connect (
        ui.deleteAttributeButton, SIGNAL (clicked()),
        this, SLOT (deleteAttributeClicked()));

    */
    ui.addAttributeButton->hide();
    ui.deleteAttributeButton->hide();
}

void BranchPropertyEditor::setSignalsEnabled(const bool b)
{
    return;

    QMutableListIterator <QWidget*> it(controlWidgets);
    while (it.hasNext()) {
        QWidget *w = it.next();
        w->blockSignals(b);
    }
}
