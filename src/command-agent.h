#ifndef COMMAND_AGENT_H
#define COMMAND_AGENT_H

#include <QObject>
#include <QProcess>
#include <QStringList>

/*! \brief Run an external command for the scripting engine.

    CommandAgent wraps a QProcess and can run a command either
    synchronously (blocking, used to return the result directly to a
    script) or in the background. In background mode the agent emits
    finished(), logs its result via mainWindow and then deletes itself,
    so the main application can keep running while the command executes.

    The background path is the place to later hook up a callback or
    message queue towards the script engine, so results of background
    commands can be accessed asynchronously.
*/

class CommandAgent : public QObject {
    Q_OBJECT

  public:
    CommandAgent(QObject *parent = nullptr);
    ~CommandAgent();

    void setCommand(const QString &command);
    void setArguments(const QStringList &args);

    //! Run synchronously, blocking until the command finishes.
    //! Returns true if the process started and exited normally.
    bool runSynchronously();

    //! Start the command in the background. The agent deletes itself
    //! once the command has finished.
    //! NOT IMPLEMENTED YET: prepared infrastructure, but not wired up to
    //! the script engine (no callback / message queue for results).
    void runInBackground();

    int exitCode();
    QString output();

  signals:
    //! Emitted when a background command has finished.
    void finished(int exitCode, const QString &output);

  private slots:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

  private:
    QProcess *process;
    QString commandInt;
    QStringList argsInt;
    int exitCodeInt;
    QString outputInt;
    bool isBackgroundInt;
};

#endif
