#ifndef __ARCH_H__
#define __ARCH_H__

/* Make sure the BYTE_ORDER macro is defined */

#ifdef WIN32
# define BIG_ENDIAN  0
# define LIL_ENDIAN  1
# define BYTE_ORDER  LIL_ENDIAN
#else
# define HAVE_FTW
# ifdef __FreeBSD__
#   include <machine/endian.h>
# elif defined(sgi)
#   include <sys/endian.h>
# elif defined(sun)
#   include <sys/isa_defs.h>
#   ifndef BYTE_ORDER
#     define BIG_ENDIAN 0
#     define LIL_ENDIAN 1
#     ifdef _LITTLE_ENDIAN
#       define BYTE_ORDER LIL_ENDIAN
#     else
#       define BYTE_ORDER BIG_ENDIAN
#     endif
#   endif
# elif defined(__svr4__)
#   include <sys/byteorder.h>
# elif defined(hpux)
#   include <arpa/nameser.h>
# elif defined(_AIX)
#   include <sys/machine.h>
# elif defined(darwin)
#   include<ppc/endian.h>
# else
#  include <endian.h>
# endif
#endif /* WIN32 */

/**** Utility functions to detect the current environment ****/

/*
 * List of currently recognized distributions
 */
typedef enum {
	DISTRO_NONE = 0, /* Unrecognized */
	DISTRO_REDHAT,
	DISTRO_MANDRAKE,
	DISTRO_SUSE,
	DISTRO_DEBIAN,
	DISTRO_SLACKWARE,
	DISTRO_GENTOO,
	DISTRO_CALDERA,
	DISTRO_LINUXPPC,
	DISTRO_YELLOWDOG,
	DISTRO_TURBO,
	DISTRO_FREEBSD,
	DISTRO_SOLARIS,
	DISTRO_HPUX,
	DISTRO_IRIX,
	DISTRO_SCO,
	DISTRO_AIX,
	DISTRO_DARWIN,
	NUM_DISTRIBUTIONS
} distribution;

/* Map between the distro code and its real name */
extern const char *distribution_name[NUM_DISTRIBUTIONS], *distribution_symbol[NUM_DISTRIBUTIONS];

/* Detect the distribution type and version */
extern distribution detect_distro(int *maj_ver, int *min_ver);

/* Function to detect the current architecture */
extern const char *detect_arch(void);

/* Returns the OS string */
extern const char *detect_os(void);

/* Function to detect the current version of libc */
extern const char *detect_libc(void);

/* Function that returns the current user's home directory */
extern const char *detect_home(void);

#endif /* __ARCH_H__ */
