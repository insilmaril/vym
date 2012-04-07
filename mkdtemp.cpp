#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <io.h>
#ifndef _WIN32
#include <sys/time.h>

extern "C" {
pid_t getpid (void);
}
#else
#include <windows.h>
#define getpid GetCurrentProcessId
#include <time.h>
#include <direct.h>
#endif

char *
mkdtemp(char *tmpl)
{
    // Implementation based on GLIBC implementation.

    static const char letters[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    static uint64_t value;

    const unsigned int ATTEMPTS_MIN = (62 * 62 * 62);

    int save_errno = errno;

    size_t len = strlen(tmpl);
    if (len < 6 || strcmp(&tmpl[len - 6], "XXXXXX"))
    {
        errno = EINVAL;
        return NULL;
    }

    char *XXXXXX = &tmpl[len - 6];

    uint64_t random_time_bits = time(NULL);

    value += (random_time_bits ^ getpid());

    unsigned int count;
    for (count = 0; count < ATTEMPTS_MIN; value += 7777, ++count)
    {
        uint64_t v = value;

        XXXXXX[0] = letters[v % 62];
        v /= 62;
        XXXXXX[1] = letters[v % 62];
        v /= 62;
        XXXXXX[2] = letters[v % 62];
        v /= 62;
        XXXXXX[3] = letters[v % 62];
        v /= 62;
        XXXXXX[4] = letters[v % 62];
        v /= 62;
        XXXXXX[5] = letters[v % 62];

	if (mkdir(tmpl) == 0)
        {
            errno = save_errno;
	    return tmpl;
        }

	if (errno != EEXIST)
	    return NULL;
    }

    errno = EEXIST;
    return NULL;
}
