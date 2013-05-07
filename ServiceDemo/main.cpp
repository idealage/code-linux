#include <iostream>

#include <signal.h>
#include <stdlib.h>
#include <syslog.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>

#include "TiTextLog.h"

using namespace std;

CTiTextLog log;

void daemonize(const char *cmd)
{
    umask(0);	// 重设文件权限掩码

	// Become a session leader to lose controlling TTY. (创建子进程，父进程退出)
    pid_t pid = 0;
    if ((pid = fork()) < 0)
        log.AddLogf("%s: can't fork", cmd);
    else if (pid != 0) /* parent */
        exit(0);

    setsid();

	// Ensure future opens won't allocate controlling TTYs.
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0)
        log.AddLogf("can't ignore SIGHUP");

    if ((pid = fork()) < 0)
        log.AddLogf("%s: can't fork", cmd);
    else if (pid != 0) /* parent */
        exit(0);

    // Change the current working directory to the root so
    // we won't prevent file systems from being unmounted.
    if (chdir("/") < 0)
        log.AddLogf("can't change directory to /");

	// Get maximum number of file descriptors.
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
		log.AddLogf("%s: can't get file limit", cmd);

    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;

	// Close all open file descriptors.
    for (int i = 0; i < (int)rl.rlim_max; i++)
        ::close(i);

	// Attach file descriptors 0, 1, and 2 to /dev/null.
    int fd0 = open("/dev/null", O_RDWR);
    int fd1 = dup(0);
    int fd2 = dup(0);

	// Initialize the log file.
    openlog(cmd, LOG_CONS, LOG_DAEMON);
    if (fd0 != 0 || fd1 != 1 || fd2 != 2)
    {
		log.AddLogf("unexpected file descriptors %d %d %d",fd0, fd1, fd2);
		syslog(LOG_ERR, "unexpected file descriptors %d %d %d",fd0, fd1, fd2);
        exit(1);
    }

	log.AddLogf("daem ok ");
    syslog(LOG_DEBUG, "daem ok ");
}

// 重新参数的信号响应
void signal_reload(int signal)
{
	time_t t = time(0);
	log.AddLogf("I received signal(%d), reload all parameters at %s\n", signal, asctime(localtime(&t)));
}

// 退出服务的信号响应
void signal_exit(int signal)
{
	time_t t = time(0);
    log.AddLogf("I received signal(%d), exit at %s\n", signal, asctime(localtime(&t)));
	exit(0);
}

int main(int argc, char* argv[])
{
	int i = 0;
	log.Init("/tmp/sd.log", true);

	daemonize("test");

	signal(SIGCHLD, SIG_IGN);		/* 忽略子进程退出信号，若在此之后又产生了子进程，
										如果不处理此信号，将在子进程退出后产生僵尸进程 */
	signal(SIGUSR1, signal_reload);	/* 处理SIGUSR1信号，可以定义此信号为重导参数信号
										在控制台输入: kill -s SIGUSR1 进程ID */
	signal(SIGUSR2, signal_exit);	/* 处理SIGUSR2信号，可以定义此信号为退出信号
										在控制台输入: kill -s SIGUSR2 进程ID */

	while(true)
	{
		log.AddLogf("log -> %d", i++);
		cout << "log -> " << i << endl;
		sleep(2);
	}

	return 0;
}
