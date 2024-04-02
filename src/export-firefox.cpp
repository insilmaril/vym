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
        key = ai->key();

        // Rewrite some values, which maybe have been modified in map
        if (key == "index")
            ai->setValue(bi->num());
        else if (key == "uri" && bi->hasUrl())
            ai->setValue(bi->url());
        else if (key == "title" && !bi->headingPlain().isEmpty())
            ai->setValue(bi->headingPlain());

        // Export values
        if (key == "postData")
            jsobj[key] = QJsonValue::Null; 
        else if (ai->value().typeName() == "QDateTime") 
            jsobj[key] = QJsonValue(ai->value().toDateTime().toMSecsSinceEpoch() * 1000);
        else if (ai->value().typeName() == "QString") 
            jsobj[key] = ai->value().toString();
        else if (ai->value().typeName() == "Integer") 
        {
            jsobj[key] = QJsonValue(ai->value().toInt());
        }
        else
            qWarning() << "ExportFirefox  Unknown attribute type in " << bi->headingPlain() << "Key: " << key;
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

    result = ExportBase::Success;
    completeExport();
}
