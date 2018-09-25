#include "puansonchecker.h"
#include <QApplication>
#include <QtGlobal>
#include <QtDebug>
#include <QTextStream>
#include <QTextCodec>
#include <QLocale>
#include <QTime>
#include <QFile>
#include <cstdio>

const QString logFilePath = "debug.log";
static bool logToFile = true;

void customMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    using namespace std;

    QHash<QtMsgType, QString> msgLevelHash({{QtDebugMsg, "Debug"}, {QtInfoMsg, "Info"}, {QtWarningMsg, "Warning"}, {QtCriticalMsg, "Critical"}, {QtFatalMsg, "Fatal"}});
    QByteArray localMsg = msg.toLocal8Bit();
    QTime time = QTime::currentTime();
    QString formattedTime = time.toString("hh:mm:ss.zzz");
    QByteArray formattedTimeMsg = formattedTime.toLocal8Bit();
    QString logLevelName = msgLevelHash[type];
    QByteArray logLevelMsg = logLevelName.toLocal8Bit();

    if (logToFile) {
        QString txt = QString("%1 %2: %3 (%4)").arg(formattedTime, logLevelName, msg,  context.file);
        QFile outFile(logFilePath);
        outFile.open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream ts(&outFile);
        ts << txt << endl;
        outFile.close();
    } else {
        fprintf(stderr, "%s %s: %s (%s:%u, %s)\n", formattedTimeMsg.constData(), logLevelMsg.constData(), localMsg.constData(), context.file, context.line, context.function);
        fflush(stderr);
    }

    if (type == QtFatalMsg)
        abort();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //qInstallMessageHandler(customMessageOutput); // custom message handler for debugging

    PuansonChecker::getInstance(&a);

    return a.exec();
}
