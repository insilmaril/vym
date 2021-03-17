#include "mainwindow.h"
#include <QMessageBox>

#include "export-firefox.h"

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
    qDebug() << "FF build list bi = " << bi;
    QJsonObject jobj;
    
    // Loop over children branches
    QJsonArray jarray;

    for (int i = 0; i < bi->branchCount(); i++)
        jarray.append(buildList(bi->getBranchNum(i)));

    jsobj["foo"] = "bar";
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

    QString out;   // FIXME-0  needed?

    // Select bookmark main branch
    model->select("mc:0,bo:0");

    BranchItem *bi = model->getSelectedBranch();
    if (!bi) return;

    // Loop over all branches below "Bookmarks"
    QString s;
    QJsonObject jobj;
    QJsonArray jarray;

    qDebug() << "XP FF a";
    for (int i = 0; i < bi->branchCount(); i++)
        jarray.append(buildList(bi->getBranchNum(i)));
    qDebug() << "XP FF b";

    jobj["root"] = jarray;

    qDebug() << "XP FF c";
    file.write(QJsonDocument(jobj).toJson());
    file.close();

    qDebug() << "XP FF d";
    //QClipboard *clipboard = QGuiApplication::clipboard();
    //clipboard->setText(out);

    //displayedDestination = filePath;

    success = true;
    completeExport();
}
