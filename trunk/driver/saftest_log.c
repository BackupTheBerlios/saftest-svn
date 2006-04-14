#include "saftest_common.h"
#include "saftest_stdarg.h"

static FILE *daemon_log_fp = NULL;

#define MAX_TIME_FORMAT_SIZE 1024

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
    char fmt_time[MAX_TIME_FORMAT_SIZE];
    struct tm *cur_tm;
    time_t cur_time;
    char va_buf[BUF_SIZE+1];
    char final_buf[BUF_SIZE+1];

    time(&cur_time);
    cur_tm = localtime(&cur_time);
    strftime(fmt_time, MAX_TIME_FORMAT_SIZE, "%b %d %T", cur_tm);

    va_start(ap, format);
    SAFTEST_SAVE_VA(aq, ap);
    vsnprintf(va_buf, BUF_SIZE, format, aq);
    sprintf(final_buf, "%s %s", fmt_time, va_buf);
    if (NULL != daemon_log_fp) {
        fprintf(daemon_log_fp, final_buf);
        fflush(daemon_log_fp);
    } else {
        fprintf(stdout, final_buf);
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
    SAFTEST_SAVE_VA(aq, ap);
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
        SAFTEST_SAVE_VA(aq, ap);
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
    SAFTEST_SAVE_VA(aq, ap);
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
