#include <QDebug>

#include "flagrowmaster.h"
#include "mainwindow.h"

extern bool debug;
extern Main *mainWindow;

/////////////////////////////////////////////////////////////////
// FlagRowMaster
/////////////////////////////////////////////////////////////////
FlagRowMaster::FlagRowMaster()
{
    // qDebug()<< "Const FlagRowMaster ()";
    toolbar = NULL;
    configureAction = NULL;
}

FlagRowMaster::~FlagRowMaster()
{
    // qDebug()<< "Destr FlagRowMaster    toolbar=" << toolbar;
}

Flag *FlagRowMaster::createFlag(const QString &path)
{
    Flag *flag = new Flag;
    flag->load(path);
    flags.append(flag);

    return flag;
}

void FlagRowMaster::createConfigureAction()
{
    if (!toolbar)
        return;

    QAction *a =
        new QAction(QIcon(":/configure-plus.svg"), QString("add flag"));
    a->setCheckable(false);
    a->connect(a, SIGNAL(triggered()), mainWindow, SLOT(addUserFlag()));

    toolbar->addAction(a);
    configureAction = a;
}

void FlagRowMaster::addActionToToolbar(QAction *a)
{
    if (!toolbar || !a)
        return;

    if (configureAction)
        toolbar->insertAction(configureAction, a);
    else
        toolbar->addAction(a);
}

Flag *FlagRowMaster::findFlagByUid(const QUuid &uid)
{
    int i = 0;
    while (i <= flags.size() - 1) {
        if (flags.at(i)->getUuid() == uid)
            return flags.at(i);
        i++;
    }
    return NULL;
}

Flag *FlagRowMaster::findFlagByName(const QString &name)
{
    int i = 0;
    while (i <= flags.size() - 1) {
        if (flags.at(i)->getName() == name)
            return flags.at(i);
        i++;
    }
    qDebug() << "FR::findFlagByName failed for name " << name;
    return NULL;
}

void FlagRowMaster::resetUsedCounter()
{
    for (int i = 0; i < flags.size(); ++i)
        flags.at(i)->setUsed(false);
}

QString FlagRowMaster::saveDef(WriteMode mode)
{
    // Write definitions of flags

    QString s = "\n";

    for (int i = 0; i < flags.size(); ++i)
        if ((mode == AllFlags) || (mode == UsedFlags && flags.at(i)->isUsed()))
            s += flags.at(i)->getDefinition(prefix);

    return s;
}

void FlagRowMaster::saveDataToDir(const QString &tmpdir, WriteMode mode)
{
    // Save icons to dir, if verbose is set (xml export)
    // and I am a master
    // and this standardflag is really used somewhere.
    // Userflags are written anyway (if master flagrow)

    for (int i = 0; i < flags.size(); ++i)
        if ((mode == AllFlags) || (mode == UsedFlags && flags.at(i)->isUsed())) {
            qDebug() << "Writing flag " << flags.at(i)->getName() << " to " << tmpdir;
            flags.at(i)->saveDataToDir(tmpdir);
        }
}

void FlagRowMaster::setName(const QString &n) { rowName = n; }

void FlagRowMaster::setPrefix(const QString &p) { prefix = p; }

QString FlagRowMaster::getName() { return rowName; }

void FlagRowMaster::setToolBar(QToolBar *tb) { toolbar = tb; }

void FlagRowMaster::setEnabled(bool b) { toolbar->setEnabled(b); }

void FlagRowMaster::updateToolBar(QList<QUuid> activeUids)
{
    if (toolbar) {
        for (int i = 0; i < flags.size(); ++i)
            flags.at(i)->getAction()->setChecked(false);

        for (int i = 0; i < flags.size(); ++i) {
            int n = activeUids.indexOf(flags.at(i)->getUuid());
            if (n >= 0)
                flags.at(i)->getAction()->setChecked(true);
        }
    }
}
