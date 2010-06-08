/* $Id: arch.c,v 1.17 2004/09/02 03:21:21 megastep Exp $ */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <string.h>
#include <ctype.h>

#include "arch.h"


/* Function to detect the current architecture */
const char *detect_arch(void)
{
    const char *arch;

    /* See if there is an environment override */
    arch = getenv("SETUP_ARCH");
    if ( arch == NULL ) {
#ifdef __i386
        arch = "x86";
#elif defined(__x86_64__)
        arch = "amd64";
#elif defined(powerpc64) || defined(__powerpc64__)
        arch = "ppc64";
#elif defined(powerpc) || defined(_POWER) || defined(__ppc__) || defined(__POWERPC__)
        arch = "ppc";
#elif defined(__alpha__)
        arch = "alpha";
#elif defined(__sparc__)
#ifdef __sun 
	/* SunOS / Solaris */
	arch = "sparc";
#else /* Linux ? */
        arch = "sparc64";
#endif
#elif defined(hppa)
	arch = "hppa";
#elif defined(__arm__)
        arch = "arm";
#elif defined(mips)
	arch = "mips";
#else
        arch = "unknown";
#endif
    }
    return arch;
}

/* Function to detect the operating system */
const char *detect_os(void)
{
    static struct utsname buf;

    uname(&buf);
	/* Exception: OpenUnix is really SCO */
	if ( !strcmp(buf.sysname, "OpenUNIX") ) {
		return "SCO_SV";
	}
    return buf.sysname;
}

/* Function to detect the current version of libc */
const char *detect_libc(void)
{
#ifdef __linux
    static const char *libclist[] = {
        "/lib/libc.so.6",
        "/lib/libc.so.6.1",
        NULL
    };
    int i;
    const char *libc;
    const char *libcfile;

    /* See if there is an environment override */
    libc = getenv("SETUP_LIBC");
    if ( libc != NULL ) {
        return(libc);
    }

    /* Look for the highest version of libc */
    for ( i=0; libclist[i]; ++i ) {
        if ( access(libclist[i], F_OK) == 0 ) {
            break;
        }
    }
    libcfile = libclist[i];

    if ( libcfile ) {
      char buffer[1024];
      snprintf( buffer, sizeof(buffer), 
           "fgrep GLIBC_2.1 %s 2>&1 >/dev/null",
           libcfile );
      
      if ( system(buffer) == 0 )
		  return "glibc-2.1";
      else
		  return "glibc-2.0";
    }
    /* Default to version 5 */
    return "libc5";
#else
	return "glibc-2.1";
#endif
}

/* Function that returns the current user's home directory */
const char *detect_home(void)
{
    static const char *home = NULL;

    /* First look up the user's home directory in the password file,
       based on effective user id.
     */
    if ( ! home ) {
        struct passwd *pwent;
        pwent = getpwuid(geteuid());
        if ( pwent ) {
            /* Small memory leak, don't worry about it */
            home = strdup(pwent->pw_dir);
        }
    }

    /* We didn't find the user in the password file?
       Something wierd is going on, but use the HOME environment anyway.
     */
    if ( ! home ) {
        home = getenv("HOME");
    }

    /* Uh oh, this system is probably hosed, write to / if we can.
     */
    if ( ! home ) {
        home = "";
    }

    return home;
}

/* Locate a version number in the string */
static void find_version(const char *file,  int *maj, int *min)
{
	char line[256], *str, *s;
	FILE *f = fopen(file, "r");

	*maj = *min = 0;
	if ( f ) {
		for(;;) { /* Get the first non-blank line */
			s = str = fgets(line, sizeof(line), f);
			if ( s ) {
				while( *s ) {
					if ( ! isspace((int)*s) )
						goto outta_here; /* Ugly, but gets us out of 2 loops */
					s ++;
				}
			} else {
				break;
			}
		}
		
	outta_here:
		if ( str ) {
			/* Skip anything that's not a number */
			while ( *str && !isdigit((int)*str) ) {
				++str;
			}
			
			if ( *str ) {
				sscanf(str, "%d.%d", maj, min);
			}
		}
		fclose(f);
	}
}

/* Locate a string in the file */
static int find_string(const char *file, const char *tofind)
{
	int ret = 0;
	FILE *f = fopen(file, "r");

	if ( f ) {
		char line[256], *str;
		/* Read line by line */
		while ( (str = fgets(line, sizeof(line), f)) != NULL ) {
			if ( strstr(str, tofind) ) {
				ret = 1;
				break;
			}
		}
		fclose(f);
	}
	return ret;
}

const char *distribution_name[NUM_DISTRIBUTIONS] = {
	"N/A",
	"RedHat Linux (or similar)",
	"Mandrake Linux",
	"SuSE Linux",
	"Debian GNU/Linux (or similar)",
	"Slackware",
	"Gentoo",
	"Caldera OpenLinux",
	"Linux/PPC",
	"Yellow Dog Linux",
	"TurboLinux",
	"FreeBSD",
	"Sun Solaris",
	"HP-UX",
	"SGI IRIX",
	"SCO UnixWare/OpenServer",
	"IBM AIX",
	"MacOS X / Darwin"
};

const char *distribution_symbol[NUM_DISTRIBUTIONS] = {
	"none",
	"redhat",
	"mandrake",
	"suse",
	"debian",
	"slackware",
	"gentoo",
	"caldera",
	"linuxppc",
	"yellowdog",
	"turbo",
	"freebsd",
	"solaris",
	"hpux",
	"irix",
	"sco",
	"aix",
	"darwin"
};

/* Detect the distribution type and version */
distribution detect_distro(int *maj_ver, int *min_ver)
{
#if defined(__sun) && defined(__svr4__)
	struct utsname n;
	uname(&n);
	sscanf(n.release, "%d.%d", maj_ver, min_ver);
	return DISTRO_SOLARIS;
#elif defined(__hpux)
	struct utsname n;
	uname(&n);
	sscanf(n.release, "%*c.%d.%d", maj_ver, min_ver);
	return DISTRO_HPUX;
#elif defined(sgi)
	struct utsname n;
	uname(&n);
	sscanf(n.release, "%d.%d", maj_ver, min_ver);
	return DISTRO_IRIX;
#elif defined(_AIX)
	struct utsname n;
	uname(&n);
	sscanf(n.version, "%d", maj_ver);
	sscanf(n.release, "%d", min_ver);
	return DISTRO_AIX;
#elif defined(sco)
	struct utsname n;
	uname(&n);
	sscanf(n.release, "%d.%d", maj_ver, min_ver);
	return DISTRO_SCO;
#elif defined(__FreeBSD__)
	struct utsname n;
	uname(&n);
	sscanf(n.release, "%d.%d", maj_ver, min_ver);
	return DISTRO_FREEBSD;
#elif defined(__APPLE__)
	struct utsname n;
	uname(&n);
	sscanf(n.release, "%d.%d", maj_ver, min_ver);
	return DISTRO_DARWIN;
#else
	if ( !access("/etc/mandrake-release", F_OK) ) {
		find_version("/etc/mandrake-release", maj_ver, min_ver); 
		return DISTRO_MANDRAKE;
	} else if ( !access("/etc/SuSE-release", F_OK) ) {
		find_version("/etc/SuSE-release", maj_ver, min_ver); 
		return DISTRO_SUSE;
		/* Look for YellowDog Linux */
	} else if ( !access("/etc/yellowdog-release", F_OK) ) {
		find_version("/etc/yellowdog-release", maj_ver, min_ver); 
		return DISTRO_YELLOWDOG;
	} else if ( !access("/etc/turbolinux-release", F_OK) ) {
		find_version("/etc/turbolinux-release", maj_ver, min_ver); 
		return DISTRO_TURBO;
	} else if ( !access("/etc/redhat-release", F_OK) ) {
		find_version("/etc/redhat-release", maj_ver, min_ver); 
#if defined(PPC) || defined(powerpc)
		/* Look for Linux/PPC */
		if ( find_string("/etc/redhat-release", "Linux/PPC") ) {
			return DISTRO_LINUXPPC;
		}
#endif
		return DISTRO_REDHAT;
	} else if ( !access("/etc/debian_version", F_OK) ) {
		find_version("/etc/debian_version", maj_ver, min_ver); 
		return DISTRO_DEBIAN;
	} else if ( !access("/etc/slackware-version", F_OK) ) {
		find_version("/etc/slackware-version", maj_ver, min_ver);
		return DISTRO_SLACKWARE;
	} else if ( !access("/etc/gentoo-release", F_OK) ) {
		find_version("/etc/gentoo-release", maj_ver, min_ver);
		return DISTRO_GENTOO;
	} else if ( find_string("/etc/issue", "OpenLinux") ) {
		find_version("/etc/issue", maj_ver, min_ver);
		return DISTRO_CALDERA;
	}
	return DISTRO_NONE; /* Couldn't recognize anything */
#endif
}
