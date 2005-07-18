#include "saftest_common.h"
#include "saftest_log.h"
#include "saftest_comm.h"

void
block_signals()
{
    sigset_t             sigset;

    sigfillset(&sigset);
    sigdelset(&sigset, SIGABRT);
    sigdelset(&sigset, SIGSEGV);
    sigdelset(&sigset, SIGINT);
    if (sigprocmask(SIG_SETMASK, &sigset, NULL) != 0) {
        ais_test_abort("Error blocking signals\n");
    }
}

void
unblock_signals()
{
    sigset_t             sigset;

    sigemptyset(&sigset);
    if (sigprocmask(SIG_SETMASK, &sigset, NULL) != 0) {
        ais_test_abort("Error unblocking signals\n");
    }
}

void
setup_core_dump(const char *run_dir)
{
    struct rlimit lim;
    int ret = 0;

    lim.rlim_cur = RLIM_INFINITY;
    lim.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &lim);

    ret = chdir(run_dir);
    if (ret != 0) {
        err_exit("Unable to change to run directory %s: %s\n",
                 run_dir, strerror(errno));
    }
}

int
write_pid_file(const char *pid_file)
{
    FILE *fp;
    pid_t pid;
    int chmod_status;

    fp = fopen(pid_file, "w");
    if (fp == NULL) {
        ais_test_log("Failed to open %s: %s\n",
                           pid_file, strerror(errno));
        return(-1);
    }
    chmod_status = chmod(pid_file, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (chmod_status != 0) {
        ais_test_log("Failed to change permissions on %s: %s\n",
                           pid_file, strerror(errno));
        return(-1);
    }
    pid = getpid();
    fprintf(fp, "%u\n", pid);
    fclose(fp);
    return(0);
}

void
clear_pid_file(const char *pid_file)
{
    unlink(pid_file);
}

void
daemonize(int want_fork, const char *pid_file)
{
    pid_t pid;
    int fd;

    if (want_fork) {
        if ((pid = fork()) < 0) {
            err_exit("cannot fork into background: %s\n",
                    strerror(errno));
        }

        /*
        * parent
        */
        if (pid) {
            exit(0);
        }
    }

    (void) setsid();

    write_pid_file(pid_file);

    /*
     * We're the child.
     *
     * Close all open file descriptors
     */
    for (fd = 0; fd < getdtablesize(); fd++) {
        close(fd);
    }

    /*
     * Open the standard io descriptors (stdin, stdout, stderr)
     * to a harmless device.  So, if some misguided routine
     * attempts to write to one of them while we are in the background,
     * nothing will happen.
     */

    /*
     * This will be fd 0, stdin.
     */
    fd = open("/dev/null", O_RDWR);

    if (fd < 0) {
        err_exit("cannot open /dev/null i/o: %s\n",
                 strerror(errno));
    }

    if (dup2(fd, 1) == -1) {
        err_exit("cannot redirect standard i/o: %s\n",
                 strerror(errno));
    }

    if (dup2(fd, 2) == -1) {
        err_exit("cannot redirect standard i/o: %s\n",
                 strerror(errno));
    }

    block_signals();
}

FILE *
setup_log_file(const char *log_file)
{
    FILE *fp;
    fp = fopen(log_file, "a");
    if (NULL == fp) {
        err_exit("Unable to open log file %s\n",
                log_file);
    }
    ais_test_log_set_fp(fp);
    return(fp);
}

void 
saftest_driver_init(const char *run_path)
{
    setup_core_dump(run_path);
}

void 
saftest_driver_client_init(const char *run_path)
{
    saftest_driver_init(run_path);
}

