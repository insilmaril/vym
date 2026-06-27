#include "command-agent.h"

#include "mainwindow.h" // logInfo()

extern Main *mainWindow;

CommandAgent::CommandAgent(QObject *parent) : QObject(parent)
{
    process = new QProcess(this);

    // Combine stdout and stderr, so a script gets the complete output
    process->setProcessChannelMode(QProcess::MergedChannels);

    exitCodeInt = -1;
    isBackgroundInt = false;
}

CommandAgent::~CommandAgent() {}

void CommandAgent::setCommand(const QString &command) { commandInt = command; }

void CommandAgent::setArguments(const QStringList &args) { argsInt = args; }

bool CommandAgent::runSynchronously()
{
    isBackgroundInt = false;

    mainWindow->logInfo("Executing (foreground): " + commandInt + " " +
                            argsInt.join(" "),
                        __func__);

    process->start(commandInt, argsInt);

    if (!process->waitForStarted()) {
        outputInt = QString("Could not start command '%1'").arg(commandInt);
        exitCodeInt = -1;
        return false;
    }

    // Block until the command has finished (no timeout)
    process->waitForFinished(-1);

    outputInt = QString::fromLocal8Bit(process->readAll());

    if (process->exitStatus() != QProcess::NormalExit) {
        outputInt += QString("\nCommand '%1' did not exit normally")
                         .arg(commandInt);
        exitCodeInt = -1;
        return false;
    }

    exitCodeInt = process->exitCode();
    return true;
}

void CommandAgent::runInBackground()
{
    // NOT IMPLEMENTED YET: This path is prepared, but not yet used by the
    // scripting engine, see VymModelWrapper::execute(). The finished() signal
    // below is the intended hook for returning results asynchronously.
    isBackgroundInt = true;

    connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this,
            SLOT(processFinished(int, QProcess::ExitStatus)));

    mainWindow->logInfo("Executing (background): " + commandInt + " " +
                            argsInt.join(" "),
                        __func__);

    process->start(commandInt, argsInt);
}

void CommandAgent::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);

    exitCodeInt = exitCode;
    outputInt = QString::fromLocal8Bit(process->readAll());

    mainWindow->logInfo(
        QString("Background command '%1' finished with exit code %2")
            .arg(commandInt)
            .arg(exitCode),
        __func__);

    emit finished(exitCode, outputInt);

    // Background agent is no longer needed
    deleteLater();
}

int CommandAgent::exitCode() { return exitCodeInt; }

QString CommandAgent::output() { return outputInt; }
