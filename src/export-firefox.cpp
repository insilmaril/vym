#include <QMessageBox>

#include "export-firefox.h"

#include "attributeitem.h"
#include "mainwindow.h"

extern QString vymName;
extern Main *mainWindow;

ExportFirefox::ExportFirefox()
{
    exportName = "Firefox";
    filter = "JSON (*.json);;All (* *.*)";
    caption = vymName + " -" + QObject::tr("Export as Firefox bookmarks");
}

QJsonObject ExportFirefox::buildList(BranchItem *bi)
{
    // Loop over children branches
    QJsonObject jsobj;
    QJsonArray jarray;

    if (bi->branchCount() > 0 ) {
        for (int i = 0; i < bi->branchCount(); i++)
            jarray.append(buildList(bi->getBranchNum(i)));

        jsobj["children"] = jarray;
    }

    QString key;
    AttributeItem *ai;
    for (int i = 0; i < bi->attributeCount(); i++) {
        ai =bi->getAttributeNum(i);
        key = ai->getKey();

        // Rewrite some values, which maybe have been modified in map
        if (key == "index")
            ai->setValue(bi->num());
        else if (key == "uri" && !bi->getURL().isEmpty())
            ai->setValue(bi->getURL());
        else if (key == "title" && !bi->getHeadingPlain().isEmpty())
            ai->setValue(bi->getHeadingPlain());

        // Export values
        if (key == "postData")
            jsobj[key] = QJsonValue::Null; 
        else if (ai->getAttributeType() == AttributeItem::DateTime) 
            jsobj[key] = QJsonValue(ai->getValue().toDateTime().toMSecsSinceEpoch() * 1000);
        else if (ai->getAttributeType() == AttributeItem::String)
            jsobj[key] = ai->getValue().toString();
        else if (ai->getAttributeType() == AttributeItem::Integer) 
        {
            jsobj[key] = QJsonValue(ai->getValue().toInt());
        }
        else
            qWarning() << "ExportFirefox  Unknown attribute type in " << bi->getHeadingPlain() << "Key: " << key;
    }

    return jsobj;
}

void ExportFirefox::doExport()
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(
            0, QObject::tr("Critical Export Error"),
            QObject::tr("Could not export as Firefox bookmarks to %1").arg(filePath));
        mainWindow->statusMessage(QString(QObject::tr("Export failed.")));
        return;
    }

    // Select bookmark main branch
    model->select("mc:0,bo:0");

    BranchItem *bi = model->getSelectedBranch();
    if (!bi) return;

    // Loop over all branches
    QJsonObject jsobj;
    QJsonArray jarray;

    /*
    for (int i = 0; i < bi->branchCount(); i++)
        jarray.append(buildList(bi->getBranchNum(i)));

    jsobj["children"] = jarray;
    */
    jsobj = buildList(bi);

    file.write(QJsonDocument(jsobj).toJson());
    file.close();

    displayedDestination = filePath;

    success = true;
    completeExport();
}
