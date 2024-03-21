#ifndef JIRAISSUE_H
#define JIRAISSUE_H

#include <QJsonObject>

class JiraIssue{
  public:
    JiraIssue ();  
    JiraIssue (const QJsonObject&);  
    void initFromJsonObject(const QJsonObject&);  
    void print() const;
    bool isFinished() const;
    QString assignee() const;
    QString components() const;
    QString desciption() const;
    QString fixVersions() const;
    QString issueType() const;
    QString key() const;
    QString parentKey() const;
    QString reporter() const;
    QString resolution() const;
    QString status() const;
    QString summary() const;
    QString url() const;
    QString issueLinks() const;
    QString subTasks() const;

  private:  
    QString assigneeInt;
    QString descriptionInt;
    QString keyInt;
    QString issuetypeInt;
    QString parentKeyInt;
    QString reporterInt;
    QString resolutionInt;
    QString statusInt;
    QString summaryInt;
    QStringList componentsListInt;
    QStringList fixVersionsListInt;
    QStringList subTasksListInt;
    QStringList issueLinksListInt;

    QString jiraServerInt;
};
#endif
