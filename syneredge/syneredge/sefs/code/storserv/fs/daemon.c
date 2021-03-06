
#include "daemon.h"
#include <signal.h>

static int
write_pid(pid_t pid)
{
	return 1 ;
}

static int
setup_signal_handlers(__sighandler_t sigfunc)
{
	if ( signal(SIGHUP, sigfunc) == SIG_ERR ) {
		return 0 ;
	}

	if ( signal(SIGQUIT, sigfunc) == SIG_ERR ) {
		return 0 ;
	}

	if ( signal(SIGINT, sigfunc) == SIG_ERR ) {
		return 0 ;
	}

	return 1 ;
}

static pid_t
become_daemon()
{
	pid_t pid ;

	if ( (pid = fork() ) < 0 ) {
		return -1 ;
	} else if ( pid != 0 ) {
		exit(0) ;
	}

	/*
	** We are the walrus, coo coo ka choo! (errr... child)
	*/
	setsid() ;
	chdir("/") ;
	umask(0) ;

	return pid ;
}



int
daemon_setup_sighandlers(__sighandler_t sigfunc)
{
	return setup_signal_handlers(sigfunc) ;
}

int 
daemon_start(int becomedaemon)
{
	pid_t pid ;

	if (becomedaemon) {
		pid = become_daemon() ;
		if (pid != -1) {
			write_pid(pid) ;
		}
	} else {
		write_pid(getpid()) ;
	}
}

static int
send_signal(pid_t pid, int sig)
{
	int rc ;

	rc = kill(pid, sig) ;

	if (rc < 0) {
		return 0 ;
	}

	return 1 ;
}

int 
daemon_stop(pid_t pid)
{
	return send_signal(pid, SIGQUIT) ;
}

int 
daemon_reinit(pid_t pid)
{
	return send_signal(pid, SIGHUP) ;
}

int 
daemon_send_sig(pid_t pid, int signo)
{
	return send_signal(pid, signo) ;
}
