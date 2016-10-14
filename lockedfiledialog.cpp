#include "lockedfiledialog.h"

LockedFileDialog::LockedFileDialog(QWidget* parent):QDialog (parent)
{
    ui.setupUi(this);
    ui.openReadonlyButton->setText(tr("Open readonly"));
    ui.deleteLockfileButton->setText(tr("Delete lockfile"));
}

LockedFileDialog::Result LockedFileDialog::execDialog()
{
    // Really show dialog
    if (QDialog::exec() == QDialog::Accepted)
        return OpenReadonly;
    else
        return DeleteLockfile;
}


void LockedFileDialog::setText (const QString &s)
{
    ui.warningTE->setText(s);
}

void LockedFileDialog::setCaption(const QString &s)
{
    QDialog::setWindowTitle("VYM - "+s);
}
