#include "xml-ithoughts.h"

#include <QDebug>
#include <QTextStream>

#include "branchitem.h"
#include "branch-container.h"
#include "mainwindow.h"
#include "misc.h"
#include "vymmodel.h"

extern Main *mainWindow;


// Very simple and incomplete importer for the meanwhile gone ithoughts app
// See also https://www.toketaware.com/ithoughts-help

IThoughtsReader::IThoughtsReader(VymModel* m)
    : BaseReader(m)
{
    //qDebug() << "Constr. IThoughtsReader";

    branchesTotal = 0;
    useProgress = false;

    lastBranch = nullptr;
    lastMI = nullptr;
}

bool IThoughtsReader::read(QIODevice *device)
{
    xml.setDevice(device);

    if (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("iThoughts")) {
            readIThoughtsMap();
        } else {
            xml.raiseError("No \"IThoughts\" as next element.");
        }
    }
    return !xml.error();
}

void IThoughtsReader::readIThoughtsMap()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("iThoughts"));

    branchesTotal = 0;
    branchesCounter = 0;

    if (loadMode == File::NewMap || loadMode == File::DefaultMap) {
        // Create mapCenter
        model->clear();
        lastBranch = model->getRootItem();

        readMapAttr();
    } else {
        // Imports need a dedicated branch
        lastBranch = insertBranch;

        if (loadMode == File::ImportReplace) {
            if (!lastBranch) {
                xml.raiseError("readIThoughtsMap - ImportReplace map, but nothing selected!");    // FIXME-4 not working, even if something is selected
                return;
            }

            insertPos = lastBranch->num();
            BranchItem *pb = lastBranch->parentBranch();
            if (!pb) {
                xml.raiseError("readIThoughtsMap - No parent branch for selection in ImportReplace!");
                return;
            }

            model->deleteSelection(lastBranch->getID());
            lastBranch = pb;
            loadMode = File::ImportAdd;
        } else {
            // ImportAdd
            if (insertPos < 0)
                insertPos = 0;
        }
    }

    if (!lastBranch)
        // Make sure, that mapcenters can be pasted on empty map e.g. for undo
        lastBranch = model->getRootItem();

    lastMI = lastBranch;


    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("topics") ) {
            readTopics(loadMode, insertPos);
            insertPos++;
        } else {
            raiseUnknownElementError();
            return;
        }
    }
}

void IThoughtsReader::readTopics(File::LoadMode loadModeBranch, int insertPosBranch)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("topics"));

    // While going deeper, no longer "import" but just load as usual
    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("topic")) {
            readBranchOrMapCenter(loadModeBranch, insertPosBranch);
        } else {
            raiseUnknownElementError();
            return;
        }
    }

}

void IThoughtsReader::readBranchOrMapCenter(File::LoadMode loadModeBranch, int insertPosBranch)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("topic"));

    // Create branch or mapCenter (but only for <topic>)
    if (loadModeBranch == File::NewMap)
        lastBranch = model->createBranchWhileLoading(lastBranch);
    else {
        // For Imports create branch at insertPos
        // (Here we only use ImportInsert, replacements already have
        // been done before)
        if (loadModeBranch == File::ImportAdd)
            lastBranch = model->createBranchWhileLoading(lastBranch, insertPos);
    }
    // Prepare parsing heading later
    lastMI = lastBranch;

    readBranchAttr();

    // While going deeper, no longer "import" but just load as usual
    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("topic")) {
            // Going deeper we regard incoming data as "new", no inserts/replacements
            readBranchOrMapCenter(File::NewMap, -1);
        } else {
            raiseUnknownElementError();
            return;
        }
    }

    // Empty branches may not be scrolled
    // (happens if bookmarks are imported)
    if (lastBranch->isScrolled() && lastBranch->branchCount() == 0)
        lastBranch->unScroll();

    lastBranch->updateVisuals();

    lastBranch = lastBranch->parentBranch();
    lastBranch->setLastSelectedBranch(0);
}

void IThoughtsReader::readMapAttr()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("IThoughts"));

    QString a = "author";
    QString s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        model->setMapAuthor(s);

    QString comment = "Map imported from ithoughts:\n\n";
    for (int i = 0; i < xml.attributes().count(); i++) {
        QString a = xml.attributes().at(i).name().toString();
        if (a != "author")
            comment += QString("%1: %2\n").arg(a, xml.attributes().at(i).value().toString());
    }

    model->setMapComment(comment);
}

void IThoughtsReader::readBranchAttr()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("topic"));

    branchesCounter++;
    if (useProgress)
        mainWindow->addProgressValue((float)branchesCounter / branchesTotal);

    lastMI = lastBranch;
    BranchContainer *lastBC = lastBranch->getBranchContainer();

    readOrnamentsAttr();

    QString a = "folded";
    QString s = xml.attributes().value(a).toString();
    if (s == "1")
        lastBranch->toggleScroll();

    bool ok;
    qreal r;

    a = "scaleHeading";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        r = s.toDouble(&ok);
        if (!ok) {
            xml.raiseError("Could not parse attribute " + a);
            return;
        }
        lastBC->setScaleAutoDesign(false, false);
        lastBC->setScaleHeading(r);
    }

    a = "text";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        lastBranch->setHeadingPlainText(s);
}

void IThoughtsReader::readOrnamentsAttr()
{
    Q_ASSERT(xml.isStartElement() && 
            xml.name() == QLatin1String("topic"));

    float x, y;
    bool okx, oky;

    QString s = attributeToString("position");
    if (!s.isEmpty()) {
        // Get coordinates from "{x,y}"
        s.replace("{", "");
        s.replace("}", "");
        QStringList sl = s.split(",");
        if (sl.size() < 2) {
            xml.raiseError("Couldn't split position string of item");
            return;
        }

        x = sl[0].toFloat(&okx);
        y = sl[1].toFloat(&oky);
        if (okx && oky)
            lastMI->setPos(QPointF(x, y));
        else {
            xml.raiseError("Couldn't read position of item");
            return;
        }
    }

    s = attributeToString("uuid");
    if (!s.isEmpty()) {
        // While pasting, check for existing UUID
        if (loadMode == File::ImportAdd || loadMode == File::ImportReplace) {
            bool x = model->findUuid(QUuid(s));
            if (!x)
                // Only set Uuid if not adding replacing in map - then duplicate Uuids might cause problems
                // In testing one map will import itself - no new Uuids then.
                lastMI->setUuid(s);
        } else
            lastMI->setUuid(s);
    }
}

