#include <utils.h>

void daemonize() {
	#ifndef DEBUG
	/* Our process ID and Session ID */
	pid_t pid, sid;

	/* Fork off the parent process */
	pid = fork();
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}
	/* If we got a good PID, then
	   we can exit the parent process. */
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	/* Change the file mode mask */
	umask(0);

	/* Create a new SID for the child process */
	sid = setsid();
	if (sid < 0) {
		/* Log the failure */
		ERR("Failed to create a new SID");
		exit(EXIT_FAILURE);
	}

	/* Change the current working directory */
        if ((chdir("/tmp")) < 0) {
                /* Log the failure */
                exit(EXIT_FAILURE);
        }

        /* Close out the standard file descriptors */
	close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
	#endif
}
