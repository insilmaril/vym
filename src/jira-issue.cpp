#include "jira-issue.h"

#include <QDebug>
#include <QJsonArray>

extern QTextStream vout;

JiraIssue::JiraIssue() {}

JiraIssue::JiraIssue(const QJsonObject &jsobj)
{
    initFromJsonObject(jsobj);
}

void JiraIssue::initFromJsonObject(const QJsonObject &jsobj) {
    qDebug() << "JI::initFromJSO called";

    //QJsonDocument jsdoc = QJsonDocument (jsobj);
    keyInt = jsobj["key"].toString();

    QJsonObject fields = jsobj["fields"].toObject();

    QJsonObject assigneeObj = fields["assignee"].toObject();
    assigneeInt = assigneeObj["emailAddress"].toString();

    QJsonObject reporterObj = fields["reporter"].toObject();
    reporterInt = reporterObj["emailAddress"].toString();

    QJsonObject issueTypeObj = fields["issuetype"].toObject();
    issuetypeInt  = issueTypeObj["name"].toString();

    QJsonObject resolutionObj = fields["resolution"].toObject();
    resolutionInt  = resolutionObj["name"].toString();

    QJsonObject statusObj = fields["status"].toObject();
    statusInt  = statusObj["name"].toString();

    summaryInt = fields["summary"].toString();

    QJsonArray componentsArray = fields["components"].toArray();
    QJsonObject compObj;
    for (int i = 0; i < componentsArray.size(); ++i) {
        compObj = componentsArray[i].toObject();
        componentsListInt << compObj["name"].toString();
    }

    QJsonArray fixVersionsArray = fields["fixVersions"].toArray();
    QJsonObject fixVersionsObj;
    QStringList fixVersionsList;
    for (int i = 0; i < fixVersionsArray.size(); ++i) {
        fixVersionsObj = fixVersionsArray[i].toObject();
        fixVersionsListInt << fixVersionsObj["name"].toString();
    }

    jiraServerInt = jsobj["vymJiraServer"].toString();
}

void JiraIssue::print() {
    qDebug() << "JI::print called";
    //vout << jsdoc.toJson(QJsonDocument::Indented) << Qt::endl;
    vout << "        Key: " + keyInt << Qt::endl;
    vout << "       Desc: " + summaryInt << Qt::endl;
    vout << "   Assignee: " + assigneeInt << Qt::endl;
    vout << " Components: " + components() << Qt::endl;
    vout << "  issuetype: " + issuetypeInt << Qt::endl;
    vout << "fixVersions: " + fixVersions() << Qt::endl;
    vout << "   Reporter: " + reporterInt << Qt::endl;
    vout << " Resolution: " + resolutionInt << Qt::endl;
    vout << "     Status: " + statusInt << Qt::endl;
    vout << "     Server: " + jiraServerInt;
    vout << "        Url: " + url();
}

bool JiraIssue::isFinished()
{
    QStringList solvedStates;
    solvedStates << "Verification Done";
    solvedStates << "Resolved";
    solvedStates << "Closed";

    return solvedStates.contains(statusInt);
}

QString JiraIssue::key()
{
    return keyInt;
}

QString JiraIssue::assignee()
{
    return assigneeInt;
}

QString JiraIssue::reporter()
{
    return reporterInt;
}

QString JiraIssue::issueType()
{
    return issuetypeInt;
}

QString JiraIssue::resolution()
{
    return resolutionInt;
}

QString JiraIssue::status()
{
    return statusInt;
}

QString JiraIssue::summary()
{
    return summaryInt;
}

QString JiraIssue::components()
{
    return componentsListInt.join(",");
}

QString JiraIssue::fixVersions()
{
    return fixVersionsListInt.join(",");
}

QString JiraIssue::url()
{
    return jiraServerInt + "/browse/" + keyInt;
}

