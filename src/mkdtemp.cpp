
#include <QDir>
#include <stdint.h>
#ifndef _WIN32
#include <sys/time.h>

extern "C" {
pid_t getpid(void);
}
#else
#include <windows.h>
#define getpid GetCurrentProcessId
#include <direct.h>
#include <time.h>
#endif

QString mkdtemp(QString tmpl)
{
    static const char letters[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    static uint64_t value;

    const unsigned int ATTEMPTS_MIN = (62 * 62 * 62);

    if (tmpl.length() < 6 || !tmpl.endsWith("XXXXXX")) {
        return QString();
    }

    uint64_t random_time_bits = time(nullptr);

    value += (random_time_bits ^ getpid());

    unsigned int count;
    for (count = 0; count < ATTEMPTS_MIN; value += 7777, ++count) {
        uint64_t v = value;
        QString XXXXXX;
        XXXXXX.append(letters[v % 62]);
        v /= 62;
        XXXXXX.append(letters[v % 62]);
        v /= 62;
        XXXXXX.append(letters[v % 62]);
        v /= 62;
        XXXXXX.append(letters[v % 62]);
        v /= 62;
        XXXXXX.append(letters[v % 62]);
        v /= 62;
        XXXXXX.append(letters[v % 62]);

        tmpl.replace(tmpl.length() - 6, 6, XXXXXX);
        QDir dir;
        if (dir.exists(tmpl))
            continue;
        if (dir.mkpath(tmpl)) {
            return tmpl;
        }
    }
    return QString();
}
