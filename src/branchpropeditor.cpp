#include "branchpropeditor.h"

#include <QColorDialog>

#include "attributeitem.h"
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
    model = nullptr;

    ui.tabWidget->setEnabled(false);

    QPixmap pix(16, 16);
    pix.fill(QColor(Qt::black));
    ui.framePenColorButton->setIcon(pix);
    ui.frameBrushColorButton->setIcon(pix);

    // Remember the last tab, which was used when a branch was selected
    lastSelectedBranchTab = -1;

    if (!settings.value("/mainwindow/showTestMenu", false).toBool())
        ui.tabWidget->widget(3)->hide();

    //Create Model and View to hold attributes
    attributeModel = new QStandardItemModel (1, 3, this);
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

    connect(ui.tabWidget, SIGNAL(currentChanged(int)), this,
            SLOT(indexChanged(int)));

    connectSignals();
}

void BranchPropertyEditor::updateContainerLayoutButtons()
{
    BranchContainer *bc = branchItem->getBranchContainer();

    if (bc->branchesContainerAutoLayout) {
        ui.branchesAutoLayoutCheckBox->setChecked(true);
        ui.branchesLayoutsFrame->setEnabled(false);
    } else {
        ui.branchesAutoLayoutCheckBox->setChecked(false);
        ui.branchesLayoutsFrame->setEnabled(true);
    }
    switch (bc->getBranchesContainerLayout()) {
        case Container::Vertical:
            ui.branchesLayoutVerticalButton->setChecked(true);
            break;
        case Container::Horizontal:
            ui.branchesLayoutHorizontalButton->setChecked(true);
            break;
        case Container::FloatingBounded:
            ui.branchesLayoutBoundedButton->setChecked(true);
            break;
        case Container::FloatingFree:
            ui.branchesLayoutFreeButton->setChecked(true);
            break;
        default:
            qWarning() << QString("BranchPropEditor: Unknown branches layout '%1'").arg(bc->getLayoutString(bc->getBranchesContainerLayout()));
            qDebug() << "branch=" << branchItem->getHeadingPlain();
    }

    // Layout images
    if (bc->imagesContainerAutoLayout) {
        ui.imagesAutoLayoutCheckBox->setChecked(true);
        ui.imagesLayoutsFrame->setEnabled(false);
    } else {
        ui.imagesAutoLayoutCheckBox->setChecked(false);
        ui.imagesLayoutsFrame->setEnabled(true);
    }
    switch (bc->getImagesContainerLayout()) {
        case Container::Vertical:
            ui.imagesLayoutVerticalButton->setChecked(true);
            break;
        case Container::Horizontal:
            ui.imagesLayoutHorizontalButton->setChecked(true);
            break;
        case Container::FloatingBounded:
            ui.imagesLayoutBoundedButton->setChecked(true);
            break;
        case Container::FloatingFree:
            ui.imagesLayoutFreeButton->setChecked(true);
            break;
        default:
            qWarning() << QString("BranchPropEditor: Unknown images layout '%1'").arg(bc->getLayoutString(bc->getImagesContainerLayout()));
    }
}

BranchPropertyEditor::~BranchPropertyEditor()
{
    settings.setValue("/satellite/propertywindow/geometry/size", size());
    settings.setValue("/satellite/propertywindow/geometry/pos", pos());
    settings.setValue("/satellite/propertywindow/showWithMain", isVisible());
    settings.setValue("/satellite/propertywindow/currentIndex", ui.tabWidget->currentIndex());

    delete (attributeModel);
}

void BranchPropertyEditor::setItem(TreeItem *ti)
{
    disconnectSignals();    // FIXME-2 why complete disconnect? To avoid recursive calls when (pre-)setting values?
    if (!ti)
        ui.tabWidget->setEnabled(false);
    else if (ti->hasTypeBranch()) {
        branchItem = (BranchItem *)ti;
        BranchContainer *bc = branchItem->getBranchContainer();

        if (lastSelectedBranchTab >= 0)
            ui.tabWidget->setCurrentIndex(lastSelectedBranchTab);

        lastSelectedBranchTab = ui.tabWidget->currentIndex();

        // Activate the right tabs for this branch
        ui.tabWidget->setEnabled(true);
        for (int i = 0; i < 4; ++i)
            ui.tabWidget->setTabEnabled(i, true);
        ui.tabWidget->setTabEnabled(4, false);

        // Frame
        FrameContainer::FrameType t = bc->frameType();

        if (t == FrameContainer::NoFrame)
        {
            ui.frameTypeCombo->setCurrentIndex(0);
            ui.colorGroupBox->setEnabled(false);
            ui.framePaddingSpinBox->setEnabled(false);
            ui.frameWidthSpinBox->setEnabled(false);
            ui.framePaddingLabel->setEnabled(false);
            ui.frameBorderLabel->setEnabled(false);
            ui.includeChildrenCheckBox->setEnabled(false);
            ui.includeChildrenCheckBox->setEnabled(false);
        }
        else {
            QPixmap pix(16, 16);
            pix.fill(bc->framePenColor());
            ui.framePenColorButton->setIcon(pix);
            pix.fill(bc->frameBrushColor());
            ui.frameBrushColorButton->setIcon(pix);
            ui.colorGroupBox->setEnabled(true);
            ui.framePaddingSpinBox->setEnabled(true);
            ui.framePaddingSpinBox->setValue(bc->framePadding());
            ui.frameWidthSpinBox->setEnabled(true);
            ui.frameWidthSpinBox->setValue( bc->framePenWidth());
            ui.framePaddingLabel->setEnabled(true);
            ui.frameBorderLabel->setEnabled(true);
            ui.includeChildrenCheckBox->setEnabled(true);

            switch (t) {
                case FrameContainer::Rectangle:
                    ui.frameTypeCombo->setCurrentIndex(1);
                    break;
                case FrameContainer::RoundedRectangle:
                    ui.frameTypeCombo->setCurrentIndex(2);
                    break;
                case FrameContainer::Ellipse:
                    ui.frameTypeCombo->setCurrentIndex(3);
                    break;
                case FrameContainer::Cloud:
                    ui.frameTypeCombo->setCurrentIndex(4);
                    break;
                default:
                    break;
            }
            /*
            if (bc->frameIncludeChildren()) // FIXME-1 will be replaced by 2nd frame
                ui.includeChildrenCheckBox->setCheckState(Qt::Checked);
            else
                ui.includeChildrenCheckBox->setCheckState(Qt::Unchecked);
                */
        }

        // Link
        if (branchItem->getHideLinkUnselected())
            ui.hideLinkIfUnselected->setCheckState(Qt::Checked);
        else
            ui.hideLinkIfUnselected->setCheckState(Qt::Unchecked);

        // Layout branches
        updateContainerLayoutButtons();
        ui.rotationHeadingSlider->setValue(bc->getRotationHeading());
        ui.rotationInnerContentSlider->setValue(bc->getRotationContent());
        ui.rotationHeadingSpinBox->setValue(bc->getRotationHeading());
        ui.rotationInnerContentSpinBox->setValue(bc->getRotationContent());

        // Task
        Task *task = branchItem->getTask();
        if (task) {
            ui.taskPrioDelta->setEnabled(true);
            ui.taskPrioDelta->setValue(task->getPriorityDelta());
            ui.lineEditDateCreation->setText(
                task->getDateCreation().toString() + " - " +
                QString(tr("%1 days ago", "task related times"))
                    .arg(task->getAgeCreation()));
            QDateTime dt = task->getDateModification();
            if (dt.isValid()) {
                ui.lineEditDateModification->setText(
                    dt.toString() + " - " +
                    QString(tr("%1 days ago", "task related times"))
                        .arg(task->getAgeModification()));
            }
            else {
                ui.lineEditDateModification->setText("");
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
                ui.lineEditSleep->setText(s);
            }
            else {
                ui.lineEditSleep->setText("");
            }
        }
        else {
            ui.taskPrioDelta->setEnabled(false);
            ui.taskPrioDelta->setValue(0);
            ui.lineEditDateCreation->setText("");
            ui.lineEditDateModification->setText("");
            ui.lineEditSleep->setText("");
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

    // Initialize Delegate  // FIXME-3 still needed?
    //attributeDelegate.setAttributeTable (mapEditor->attributeTable());
    //ui.attributeTableView->setItemDelegate (&attributeDelegate);

    } // BranchItem
    else if (ti->getType() == TreeItem::Image) {
        ui.tabWidget->setEnabled(true);
        for (int i = 0; i < ui.tabWidget->count(); ++i)
            ui.tabWidget->setTabEnabled(i, false);
        ui.tabWidget->setTabEnabled(3, true);
        ui.tabWidget->setCurrentIndex(3);
    } // ImageItem
    else if (ti->getType() == TreeItem::Attribute) {
        ui.tabWidget->setEnabled(true);
        for (int i = 0; i < 3; ++i)
            ui.tabWidget->setTabEnabled(i, false);
        ui.tabWidget->setTabEnabled(3, true);
    }
    else {
        ui.tabWidget->setEnabled(false);
    }
    connectSignals();
}

void BranchPropertyEditor::setModel(VymModel *m)
{
    model = m;
    if (model)
        setItem(model->getSelectedItem());
    else
        ui.tabWidget->setEnabled(false);
}

void BranchPropertyEditor::frameTypeChanged(int i)
{
    if (model) {
        switch (i) {
            case 0:
                model->setFrameType(FrameContainer::NoFrame);
                break;
            case 1:
                model->setFrameType(FrameContainer::Rectangle);
                break;
            case 2:
                model->setFrameType(FrameContainer::RoundedRectangle);
                break;
            case 3:
                model->setFrameType(FrameContainer::Ellipse);
                break;
            case 4:
                model->setFrameType(FrameContainer::Cloud);
                break;
        }

        // Update data in dialog
        setItem(branchItem);
    }
}

void BranchPropertyEditor::framePenColorClicked()
{
    if (model) {
        QColor col = Qt::white;
        if (branchItem) {
            BranchContainer *bc = branchItem->getBranchContainer();
            if (bc->frameType() != FrameContainer::NoFrame)
                col = bc->framePenColor();

            col = QColorDialog::getColor(col, this);
            if (col.isValid()) {
                model->setFramePenColor(col, branchItem);
            }
        }
    }
}

void BranchPropertyEditor::frameBrushColorClicked()
{
    if (model) {
        QColor col = Qt::white;
        if (branchItem) {
            BranchContainer *bc = branchItem->getBranchContainer();
            if (bc->frameType() != FrameContainer::NoFrame)
                col = bc->frameBrushColor();

            col = QColorDialog::getColor(
                    col,
                    this,
                    tr("Background color of frame","Branch property dialog"),
                    QColorDialog::ShowAlphaChannel);
            if (col.isValid()) {
                model->setFrameBrushColor(col, branchItem);
            }
        }
    }
}

void BranchPropertyEditor::framePaddingChanged(int i)
{
    if (model)
        model->setFramePadding(i, branchItem);
}

void BranchPropertyEditor::framePenWidthChanged(int i)
{
    if (model)
        model->setFramePenWidth(i, branchItem);
}

void BranchPropertyEditor::frameIncludeChildrenChanged(int i)
{
    if (model)
        model->setFrameIncludeChildren(i, branchItem);  // FIXME-1 will be replaced by 2nd frame
}

void BranchPropertyEditor::linkHideUnselectedChanged(int i)
{
    model->setHideLinkUnselected(i);
}

void BranchPropertyEditor::childrenLayoutChanged()
{
    if (!model) return;

    if (ui.branchesAutoLayoutCheckBox->isChecked())
        model->setBranchesLayout("Auto");
    else {
        QAbstractButton *button = ui.branchesLayoutsButtonGroup->checkedButton();
        if (button) {
            if (button == ui.branchesLayoutVerticalButton)
                model->setBranchesLayout("Vertical");
            else if (button == ui.branchesLayoutHorizontalButton)
                model->setBranchesLayout("Horizontal");
            else if (button == ui.branchesLayoutBoundedButton)
                model->setBranchesLayout("FloatingBounded");
            else if (button == ui.branchesLayoutFreeButton)
                model->setBranchesLayout("FloatingFree");
            else
                qWarning() << "BPE::childrenLayout changed - unknown branches layout: " << button;
        }
    }

    if (ui.imagesAutoLayoutCheckBox->isChecked())
        model->setImagesLayout("Auto");
    else {
        QAbstractButton *button = ui.imagesLayoutsButtonGroup->checkedButton();
        if (button) {
            if (button == ui.imagesLayoutVerticalButton)
                model->setImagesLayout("Vertical");
            else if (button == ui.imagesLayoutHorizontalButton)
                model->setImagesLayout("Horizontal");
            else if (button == ui.imagesLayoutBoundedButton)
                model->setImagesLayout("FloatingBounded");
            else if (button == ui.imagesLayoutFreeButton)
                model->setImagesLayout("FloatingFree");
            else
                qWarning() << "BPE::childrenLayout changed - unknown images layout: " << button;
        }
    }

    updateContainerLayoutButtons();
}

void BranchPropertyEditor::rotationHeadingChanged(int i)    // FIXME-2 Create custom class to sync slider and spinbox and avoid double calls to models
{
    if (model)
        model->setRotationHeading(i);

    ui.rotationHeadingSlider->setValue(i);
    ui.rotationHeadingSpinBox->setValue(i);
}

void BranchPropertyEditor::rotationInnerContentChanged(int i)
{
    if (model)
        model->setRotationContent(i);

    ui.rotationInnerContentSlider->setValue(i);
    ui.rotationInnerContentSpinBox->setValue(i);
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

void BranchPropertyEditor::addAttributeClicked()
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
    // Frame
    connect(ui.framePenColorButton, SIGNAL(clicked()), this,
            SLOT(framePenColorClicked()));
    connect(ui.framePaddingSpinBox, SIGNAL(valueChanged(int)), this,
            SLOT(framePaddingChanged(int)));
    connect(ui.frameWidthSpinBox, SIGNAL(valueChanged(int)), this,
            SLOT(framePenWidthChanged(int)));
    connect(ui.frameBrushColorButton, SIGNAL(clicked()), this,
            SLOT(frameBrushColorClicked()));
    connect(ui.frameTypeCombo, SIGNAL(currentIndexChanged(int)), this,
            SLOT(frameTypeChanged(int)));
    connect(ui.includeChildrenCheckBox, SIGNAL(stateChanged(int)), this,
            SLOT(frameIncludeChildrenChanged(int)));

    // Link
    connect(ui.hideLinkIfUnselected, SIGNAL(stateChanged(int)), this,
            SLOT(linkHideUnselectedChanged(int)));

    // Layout
    connect(ui.branchesAutoLayoutCheckBox, SIGNAL(clicked()),
            this, SLOT(childrenLayoutChanged()));
    connect(ui.branchesLayoutVerticalButton, SIGNAL(clicked()),
            this, SLOT(childrenLayoutChanged()));
    connect(ui.branchesLayoutHorizontalButton, SIGNAL(clicked()),
            this, SLOT(childrenLayoutChanged()));
    connect(ui.branchesLayoutBoundedButton, SIGNAL(clicked()),
            this, SLOT(childrenLayoutChanged()));
    connect(ui.branchesLayoutFreeButton, SIGNAL(clicked()),
            this, SLOT(childrenLayoutChanged()));
    connect(ui.imagesAutoLayoutCheckBox, SIGNAL(clicked()),
            this, SLOT(childrenLayoutChanged()));
    connect(ui.imagesLayoutVerticalButton, SIGNAL(clicked()),
            this, SLOT(childrenLayoutChanged()));
    connect(ui.imagesLayoutHorizontalButton, SIGNAL(clicked()),
            this, SLOT(childrenLayoutChanged()));
    connect(ui.imagesLayoutBoundedButton, SIGNAL(clicked()),
            this, SLOT(childrenLayoutChanged()));
    connect(ui.imagesLayoutFreeButton, SIGNAL(clicked()),
            this, SLOT(childrenLayoutChanged()));

    connect(ui.rotationHeadingSlider, SIGNAL(valueChanged(int)),
            this, SLOT(rotationHeadingChanged(int)));
    connect(ui.rotationHeadingSpinBox, SIGNAL(valueChanged(int)),
            this, SLOT(rotationHeadingChanged(int)));

    // With lambda          // FIXME-3
    // connect(spinbox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), slider, &QSlider::setValue);

    connect(ui.rotationInnerContentSlider, SIGNAL(valueChanged(int)),
            this, SLOT(rotationInnerContentChanged(int)));
    connect(ui.rotationInnerContentSpinBox, SIGNAL(valueChanged(int)),
            this, SLOT(rotationInnerContentChanged(int)));

    // Tasks
    connect(ui.taskPrioDelta, SIGNAL(valueChanged(int)), this,
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

void BranchPropertyEditor::disconnectSignals()
{
    // Frame
    disconnect(ui.framePenColorButton, 0, 0, 0);
    disconnect(ui.framePaddingSpinBox, 0, 0, 0);
    disconnect(ui.frameWidthSpinBox, 0, 0, 0);
    disconnect(ui.frameBrushColorButton, 0, 0, 0);
    disconnect(ui.frameTypeCombo, 0, 0, 0);
    disconnect(ui.includeChildrenCheckBox, 0, 0, 0);

    // Link
    disconnect(ui.hideLinkIfUnselected, 0, 0, 0);

    // Layout
    disconnect(ui.branchesAutoLayoutCheckBox, 0, 0, 0);
    disconnect(ui.branchesLayoutVerticalButton, 0, 0, 0);
    disconnect(ui.branchesLayoutHorizontalButton, 0, 0, 0);
    disconnect(ui.branchesLayoutBoundedButton, 0, 0, 0);
    disconnect(ui.branchesLayoutFreeButton, 0, 0, 0);
    disconnect(ui.imagesAutoLayoutCheckBox, 0, 0, 0);
    disconnect(ui.imagesLayoutVerticalButton, 0, 0, 0);
    disconnect(ui.imagesLayoutHorizontalButton, 0, 0, 0);
    disconnect(ui.imagesLayoutBoundedButton, 0, 0, 0);
    disconnect(ui.imagesLayoutFreeButton, 0, 0, 0);
    disconnect(ui.rotationHeadingSlider, 0, 0, 0);
    disconnect(ui.rotationHeadingSpinBox, 0, 0, 0);
    disconnect(ui.rotationInnerContentSlider, 0, 0, 0);
    disconnect(ui.rotationInnerContentSpinBox, 0, 0, 0);

    // Task
    disconnect(ui.taskPrioDelta, 0, 0, 0);

    // Attributes
    disconnect (ui.addAttributeButton, 0, 0, 0);
    disconnect (ui.deleteAttributeButton, 0, 0, 0);
}
