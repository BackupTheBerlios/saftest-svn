#include "saftest_common.h"

static FILE *daemon_log_fp = NULL;

void
saftest_log_set_fp(FILE *fp)
{
    daemon_log_fp = fp;
}

void
saftest_log(const char *format, ...)
{
    va_list ap;
    va_list aq;

    char buf[BUF_SIZE+1];

    va_start(ap, format);
    /*
     * make a local copy because the pointer ap will be
     * undefined after return from vsnprintf() (Linux only)
     */
    va_copy(aq, ap);
    vsnprintf(buf, BUF_SIZE, format, aq);
    if (NULL != daemon_log_fp) {
        fprintf(daemon_log_fp, buf);
        fflush(daemon_log_fp);
    } else {
        fprintf(stdout, buf);
        fflush(stdout);
    }

    va_end(ap);
    va_end(aq);
}

void
saftest_abort(const char *format, ...)
{
    va_list ap;
    va_list aq;

    char buf[BUF_SIZE+1];

    va_start(ap, format);
    /*
     * make a local copy because the pointer ap will be
     * undefined after return from vsnprintf() (Linux only)
     */
    va_copy(aq, ap);
    vsnprintf(buf, BUF_SIZE, format, aq);
    if (NULL != daemon_log_fp) {
        fprintf(daemon_log_fp, buf);
        fflush(daemon_log_fp);
    } else {
        fprintf(stderr, buf);
        fflush(stderr);
    }

    va_end(ap);
    va_end(aq);

    abort();
}

void
saftest_assert(int assertion, const char *format, ...)
{
    va_list ap;
    va_list aq;

    char buf[BUF_SIZE+1];

    if (!assertion) {
        va_start(ap, format);
        /*
        * make a local copy because the pointer ap will be
        * undefined after return from vsnprintf() (Linux only)
        */
        va_copy(aq, ap);
        vsnprintf(buf, BUF_SIZE, format, aq);
        if (NULL != daemon_log_fp) {
            fprintf(daemon_log_fp, buf);
            fflush(daemon_log_fp);
        } else {
            fprintf(stderr, buf);
            fflush(stderr);
        }
    
        va_end(ap);
        va_end(aq);

        assert(assertion);
    }
}

void
err_exit(const char *format, ...)
{
    va_list ap;
    va_list aq;

    char buf[BUF_SIZE+1];

    va_start(ap, format);
    /*
     * make a local copy because the pointer ap will be
     * undefined after return from vsnprintf() (Linux only)
     */
    va_copy(aq, ap);
    vsnprintf(buf, BUF_SIZE, format, aq);
    if (NULL != daemon_log_fp) {
        fprintf(daemon_log_fp, buf);
        fflush(daemon_log_fp);
    } else {
        fprintf(stderr, buf);
        fflush(stderr);
    }

    va_end(ap);
    va_end(aq);

    exit(255);
}
