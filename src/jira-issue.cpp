#include "jira-issue.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>    // Only for debugging

extern QTextStream vout;

JiraIssue::JiraIssue() {}

JiraIssue::JiraIssue(const QJsonObject &jsobj)
{
    initFromJsonObject(jsobj);
}

void JiraIssue::initFromJsonObject(const QJsonObject &jsobj) {
    // Debug only
    /*
    qDebug() << __FUNCTION__ << " ***";
    QJsonDocument jsdoc = QJsonDocument (jsobj);
    vout << jsdoc.toJson(QJsonDocument::Indented) << Qt::endl;
    */

    keyInt = jsobj["key"].toString();

    QJsonObject fields = jsobj["fields"].toObject();

    QJsonObject assigneeObj = fields["assignee"].toObject();
    assigneeInt = assigneeObj["emailAddress"].toString();

    QJsonObject issueTypeObj = fields["issuetype"].toObject();
    issuetypeInt  = issueTypeObj["name"].toString();

    descriptionInt = fields["description"].toString();

    parentKeyInt = fields["parent"].toObject().value("key").toString();

    QJsonObject reporterObj = fields["reporter"].toObject();
    reporterInt = reporterObj["emailAddress"].toString();

    QJsonObject resolutionObj = fields["resolution"].toObject();
    resolutionInt  = resolutionObj["name"].toString();

    QJsonObject statusObj = fields["status"].toObject();
    statusInt  = statusObj["name"].toString();

    summaryInt = fields["summary"].toString();

    QJsonArray componentsArray = fields["components"].toArray();
    QJsonObject compObj;
    componentsListInt.clear();
    for (int i = 0; i < componentsArray.size(); ++i) {
        compObj = componentsArray[i].toObject();
        componentsListInt << compObj["name"].toString();
    }

    QJsonArray fixVersionsArray = fields["fixVersions"].toArray();
    QJsonObject fixVersionsObj;
    fixVersionsListInt.clear();
    for (int i = 0; i < fixVersionsArray.size(); ++i) {
        fixVersionsObj = fixVersionsArray[i].toObject();
        fixVersionsListInt << fixVersionsObj["name"].toString();
    }

    // For issuelinks see
    // https://developer.atlassian.com/cloud/jira/platform/issue-linking-model/

    // Experimental
    // qDebug() << "JiraIssue: issuelinks for " << keyInt;
    QJsonArray issuelinksArray = fields["issuelinks"].toArray();
    issueLinksListInt.clear();
    for (int i = 0; i < issuelinksArray.size(); ++i) {
        //qDebug() << issuelinksArray[i].toObject().value("inwardIssue")["key"].toString()
        //    << issuelinksArray[i].toObject().value("inwardIssue")["fields"]["summary"].toString();
        issueLinksListInt << issuelinksArray[i].toObject().value("inwardIssue")["key"].toString();
    }

    // Experimental
    // qDebug() << "JiraIssue: subtasks for " << keyInt;
    QJsonArray subtasksArray = fields["subtasks"].toArray();
    subTasksListInt.clear();
    for (int i = 0; i < subtasksArray.size(); ++i) {
        //qDebug() << subtasksArray[i].toObject().value("key").toString()
        //    << subtasksArray[i].toObject().value("fields")["summary"].toString();
        subTasksListInt << subtasksArray[i].toObject().value("key").toString();
    }

    // Guess Jira server from self reference
    QRegularExpression re("(.*)/rest/api");
    QRegularExpressionMatch match = re.match(jsobj["self"].toString());
    if (match.hasMatch())
        jiraServerInt = match.captured(1);
}

void JiraIssue::print() const
{
    qDebug() << "JI::print()";
    vout << "        Key: " + keyInt << Qt::endl;
    vout << "    Summary: " + summaryInt << Qt::endl;
    vout << "       Desc: " + descriptionInt.left(20) << Qt::endl;
    vout << "   Assignee: " + assigneeInt << Qt::endl;
    vout << " Components: " + components() << Qt::endl;
    vout << "  issuetype: " + issuetypeInt << Qt::endl;
    vout << "  parentKey: " + parentKeyInt << Qt::endl;
    vout << "fixVersions: " + fixVersions() << Qt::endl;
    vout << "   Reporter: " + reporterInt << Qt::endl;
    vout << " Resolution: " + resolutionInt << Qt::endl;
    vout << "     Status: " + statusInt << Qt::endl;
    vout << "     Server: " + jiraServerInt << Qt::endl; 
    vout << "        Url: " + url() << Qt::endl;
    vout << " issueLinks: " + issueLinks() << Qt::endl;
    vout << "   subTasks: " + subTasks() << Qt::endl;
}

bool JiraIssue::isFinished() const
{
    QStringList solvedStates;
    solvedStates << "Verification Done";
    solvedStates << "Resolved";
    solvedStates << "Closed";

    return solvedStates.contains(statusInt);
}

QString JiraIssue::assignee() const
{
    return assigneeInt;
}

QString JiraIssue::components() const
{
    return componentsListInt.join(",");
}

QString JiraIssue::description() const
{
    return descriptionInt;
}

QString JiraIssue::fixVersions() const
{
    return fixVersionsListInt.join(",");
}

QString JiraIssue::issueType() const
{
    return issuetypeInt;
}

QString JiraIssue::key() const
{
    return keyInt;
}

QString JiraIssue::parentKey() const
{
    return parentKeyInt;
}

QString JiraIssue::reporter() const
{
    return reporterInt;
}

QString JiraIssue::resolution() const
{
    return resolutionInt;
}

QString JiraIssue::status() const
{
    return statusInt;
}

QString JiraIssue::summary() const
{
    return summaryInt;
}

QString JiraIssue::url() const
{
    return jiraServerInt + "/browse/" + keyInt;
}

QString JiraIssue::issueLinks() const
{
    return issueLinksListInt.join(",");
}

QString JiraIssue::subTasks() const
{
    return subTasksListInt.join(",");
}

