#include "scripting.h"

#include "vymmodel.h"
#include "mainwindow.h"

extern Main *mainWindow;

VymModelWrapper::VymModelWrapper(VymModel *m)
{
    model = m;
}

void VymModelWrapper::addBranch(int pos)
{
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi)
        model->addNewBranch(selbi, pos);
}

void VymModelWrapper::setHeadingPlainText(const QString &s)
{
    model->setHeading(s);
}

QString VymModelWrapper::getHeadingPlainText()
{
    return model->getHeading().getTextASCII(); //FIXME-2 testing
}

VymWrapper::VymWrapper()
{
}

void VymWrapper::toggleTreeEditor()
{
    mainWindow->windowToggleTreeEditor();
}

