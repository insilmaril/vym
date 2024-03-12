#ifndef JIRAISSUE_H
#define JIRAISSUE_H

#include <QJsonObject>

class JiraIssue{
  public:
    JiraIssue ();  
    JiraIssue (const QJsonObject&);  
    void initFromJsonObject(const QJsonObject&);  
    void print();
    bool isFinished();
    QString key();
    QString assignee();
    QString reporter();
    QString issueType();
    QString resolution();
    QString status();
    QString summary();
    QString components();
    QString fixVersions();
    QString url();

  private:  
    QString keyInt;
    QString assigneeInt;
    QString reporterInt;
    QString issuetypeInt;
    QString resolutionInt;
    QString statusInt;
    QString summaryInt;
    QStringList componentsListInt;
    QStringList fixVersionsListInt;

    QString jiraServerInt;
};
#endif
