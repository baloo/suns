/*
 * Copyright (c) 2015 Gandi S.A.S.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>

#ifdef HAVE___PROGNAME
extern const char *__progname;
#else
#define __progname "suns"
#endif

#define NETNS_RUN_DIR "/var/run/netns"
#define PROC_NS_NET "/proc/%s/ns/net"

static inline bool
is_pid(const char *str)
{
	return (strspn(str, "0123456789") == strlen(str));
}

static void
usage()
{
	fprintf(stderr, "Usage:   %s [OPTIONS ...] COMMAND\n", __progname);
	fprintf(stderr, "-n name   Name of netns or pid.\n");
	exit(EXIT_FAILURE);
}

/* sanity_check will try to ensure the binary is only executable by owner and
 * group. There is no access control (pam, ... you name it) other than the one
 * already available in the file permissions system. This is made to avoid
 * packaging / ... mistakes which would lead to a security hole */
static inline void
sanity_check(void)
{
	char pathbuf[PATH_MAX];
	struct stat s;

	if (readlink("/proc/self/exe", pathbuf, sizeof(pathbuf)) < 0)
		err(2, "readlink /proc/self/exe failed: ");

	if (stat(pathbuf, &s) < 0)
		err(3, "stat failed: ");

	if ((s.st_mode & S_IRWXO) != 0)
		errx(1,
		     "bad permission: others have permissions on %s binary\n\t"
		     "suggested fix: chmod o-rwx %s\n",
		     __progname, pathbuf);
}

int
main(int argc, char *const argv[])
{
	int nsfd;
	char ch;
	char pathbuf[PATH_MAX];
	const char *nspath = NULL, *netspec = NULL, *options = "n:";
	uid_t ruid, euid, suid;

	sanity_check();

	while ((ch = getopt(argc, argv, options)) != -1) {
		switch (ch) {
		case 'n':
			netspec = optarg;
			break;
		}
	}

	/* netns is the only option (for now?) and thus required */
	if (netspec == NULL)
		usage();

	/* Check we have a command */
	if (optind >= argc)
		usage();

	if (is_pid(netspec)) {
		snprintf(pathbuf, sizeof(pathbuf) - 1, PROC_NS_NET, netspec);
		nspath = pathbuf;
	} else {
		const char *ptr;

		nspath = netspec;
		ptr = strchr(netspec, '/');

		if (!ptr) {
			snprintf(pathbuf, sizeof(pathbuf), "%s/%s",
				 NETNS_RUN_DIR, netspec);
		}
		nspath = pathbuf;
	}

	if (getresuid(&ruid, &euid, &suid) < 0)
		err(1, "failed to determine real user id");

	if ((nsfd = open(nspath, O_RDONLY)) < 0)
		err(1, "failed to open namespace descriptor: ");

	if (setns(nsfd, CLONE_NEWNET) != 0)
		err(1, "setns call failed: ");

	// XXX getsid ?

	if (setuid(ruid) < 0)
		err(1, "failed to setuid");

	if (execvp(argv[optind], argv + optind) < 0)
		err(1, "failed to exec program");

	/* exec returned - WTF */
	return EXIT_FAILURE;
}
