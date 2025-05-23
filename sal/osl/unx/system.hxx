/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#pragma once

#include <errno.h>
#include <grp.h>

/* Make sockets of type AF_UNIX use underlying FS rights */
#if defined(__sun) && !defined(_XOPEN_SOURCE)
#   define _XOPEN_SOURCE 500
#   include <sys/socket.h>
#   undef _XOPEN_SOURCE
#else
#   include <sys/socket.h>
#endif

#ifdef SYSV
#   include <sys/utsname.h>
#endif

#ifdef LINUX
#   ifndef __USE_GNU
#   define __USE_GNU
#   endif

#   include <pthread.h>
#   define  IORESOURCE_TRANSFER_BSD
#   define  IOCHANNEL_TRANSFER_BSD_RENO
#   define  pthread_testcancel()
#   define  NO_PTHREAD_PRIORITY
#   define  PTHREAD_SIGACTION           pthread_sigaction

#endif

#ifdef HAIKU
#   include <shadow.h>
#   include <pthread.h>
#   include <sys/file.h>
#   include <sys/ioctl.h>
#   include <sys/uio.h>
#   include <sys/un.h>
#   include <netinet/tcp.h>
#   include <dlfcn.h>
#   include <endian.h>
#   include <sys/time.h>
#   define  IORESOURCE_TRANSFER_BSD
#   define  IOCHANNEL_TRANSFER_BSD_RENO
#   define  pthread_testcancel()
#   define  NO_PTHREAD_PRIORITY
#   define  NO_PTHREAD_RTL
#   define  PTHREAD_SIGACTION           pthread_sigaction

#   define SIGIOT SIGABRT
#   define SOCK_RDM 0
//  hack: Haiku defines SOL_SOCKET as -1, but this makes GCC complain about
//  narrowing conversion
#   undef SOL_SOCKET
#   define SOL_SOCKET (sal_uInt32) -1

#endif

#if defined(ANDROID)
#   include <pthread.h>
#   include <sys/file.h>
#   include <sys/ioctl.h>
#   include <sys/uio.h>
#   include <sys/un.h>
#   include <netinet/tcp.h>
#   include <dlfcn.h>
#   include <endian.h>
#   include <sys/time.h>
#   define  IORESOURCE_TRANSFER_BSD
#   define  IOCHANNEL_TRANSFER_BSD_RENO
#   define  pthread_testcancel()
#   define  NO_PTHREAD_PRIORITY
#endif

#ifdef NETBSD
#   define  NO_PTHREAD_RTL
#endif

#ifdef FREEBSD
#   include <pthread.h>
#   include <sys/sem.h>
#   include <dlfcn.h>
#   include <sys/filio.h>
#   include <sys/ioctl.h>
#   include <sys/param.h>
#   include <sys/time.h>
#   include <sys/uio.h>
#   include <sys/exec.h>
#   include <vm/vm.h>
#   include <vm/vm_param.h>
#   include <vm/pmap.h>
#   include <vm/swap_pager.h>
#   include <sys/un.h>
#   include <netinet/tcp.h>
#   define  IORESOURCE_TRANSFER_BSD
#   include <machine/endian.h>
#   define  NO_PTHREAD_RTL
#endif

#ifdef OPENBSD
#   include <pthread.h>
#   include <sys/sem.h>
#   include <dlfcn.h>
#   include <sys/filio.h>
#   include <sys/ioctl.h>
#   include <sys/param.h>
#   include <sys/time.h>
#   include <sys/uio.h>
#   include <sys/exec.h>
#       include <sys/un.h>
#   include <netinet/tcp.h>
#       define  IORESOURCE_TRANSFER_BSD
#   include <machine/endian.h>
#      define  PTR_SIZE_T(s)   ((size_t *)&(s))
#       define  IORESOURCE_TRANSFER_BSD
#       define  IOCHANNEL_TRANSFER_BSD_RENO
#       define  pthread_testcancel()
#       define  NO_PTHREAD_PRIORITY
#       define  NO_PTHREAD_RTL
#       define  PTHREAD_SIGACTION                       pthread_sigaction
#endif

#if defined(DRAGONFLY) || defined(NETBSD)
#   include <pthread.h>
#   include <sys/sem.h>
#   include <dlfcn.h>
#   include <sys/filio.h>
#   include <sys/ioctl.h>
#   include <sys/param.h>
#   include <sys/time.h>
#   include <sys/uio.h>
#   include <sys/exec.h>
#   include <sys/un.h>
#   include <netinet/tcp.h>
#   include <machine/endian.h>
#   define  IORESOURCE_TRANSFER_BSD
#   define  IOCHANNEL_TRANSFER_BSD_RENO
#endif

#ifdef __sun
#   include <shadow.h>
#   include <sys/un.h>
#   include <stropts.h>
#   include <pthread.h>
#   include <netinet/tcp.h>
#   include <sys/filio.h>
#   include <dlfcn.h>
#   include <sys/isa_defs.h>
#   define  IORESOURCE_TRANSFER_SYSV
#   define  IOCHANNEL_TRANSFER_BSD
#   define  LIBPATH "LD_LIBRARY_PATH"
#endif

#ifdef MACOSX
#define TimeValue CFTimeValue      // Do not conflict with TimeValue in sal/inc/osl/time.h
#include <Carbon/Carbon.h>
#undef TimeValue
#   include <dlfcn.h>
#   include <pthread.h>
#   include <sys/file.h>
#   include <sys/ioctl.h>
#   include <sys/uio.h>
#   include <sys/un.h>
#   include <netinet/tcp.h>
#   include <machine/endian.h>
#   include <sys/time.h>
#   include <mach-o/dyld.h>
#   define  IOCHANNEL_TRANSFER_BSD_RENO
#   define  NO_PTHREAD_RTL
/* for NSGetArgc/Argv/Environ */
#       include <crt_externs.h>
int macxp_resolveAlias(char *path, int buflen);
#endif

#ifdef IOS
#   include <dlfcn.h>
#   include <pthread.h>
#   include <sys/file.h>
#   include <sys/ioctl.h>
#   include <sys/uio.h>
#   include <sys/un.h>
#   include <netinet/tcp.h>
#   include <machine/endian.h>
#   include <sys/time.h>
#   define  IOCHANNEL_TRANSFER_BSD_RENO
#   define  NO_PTHREAD_RTL
#endif

#ifdef EMSCRIPTEN
#   include <pthread.h>
#   include <sys/file.h>
#   include <sys/ioctl.h>
#   include <sys/uio.h>
#   include <sys/un.h>
#   include <netinet/tcp.h>
#   include <dlfcn.h>
#   include <endian.h>
#   include <sys/time.h>
#   define  IORESOURCE_TRANSFER_BSD
#   define  IOCHANNEL_TRANSFER_BSD_RENO
#   define  pthread_testcancel()
#   define  NO_PTHREAD_PRIORITY
#   define INIT_GROUPS(name, gid) false
#endif

#if !defined(_WIN32)  && \
    !defined(LINUX)   && !defined(NETBSD) && !defined(FREEBSD) && \
    !defined(__sun) && !defined(MACOSX) && \
    !defined(OPENBSD) && !defined(DRAGONFLY) && \
    !defined(IOS) && !defined(ANDROID) && \
    !defined(HAIKU) && !defined(EMSCRIPTEN)
#   error "Target platform not specified!"
#endif

#ifndef PTR_FD_SET
#   define PTR_FD_SET(s)                (&(s))
#endif

#ifndef NORMALIZE_TIMESPEC
#   define NORMALIZE_TIMESPEC(timespec) \
          timespec . tv_sec  += timespec . tv_nsec / 1000000000; \
          timespec . tv_nsec %= 1000000000;
#endif

#ifndef SET_TIMESPEC
#   define SET_TIMESPEC(timespec, sec, nsec) \
          timespec . tv_sec  = (sec);  \
          timespec . tv_nsec = (nsec); \
        NORMALIZE_TIMESPEC(timespec);
#endif

#ifndef SLEEP_TIMESPEC
#   define SLEEP_TIMESPEC(timespec) nanosleep(&timespec, nullptr)
#endif

#ifndef INIT_GROUPS
#   define  INIT_GROUPS(name, gid)  ((setgid((gid)) == 0) && (initgroups((name), (gid)) == 0))
#endif

#ifndef PTHREAD_NONE
#   define PTHREAD_NONE                 _pthread_none_
#   ifndef PTHREAD_NONE_INIT
#       define PTHREAD_NONE_INIT        ((pthread_t)-1)
#   endif
#endif

#ifndef PTHREAD_ATTR_DEFAULT
#   define PTHREAD_ATTR_DEFAULT         nullptr
#endif
#ifndef PTHREAD_MUTEXATTR_DEFAULT
#   define PTHREAD_MUTEXATTR_DEFAULT    nullptr
#endif
#ifndef PTHREAD_CONDATTR_DEFAULT
#   define PTHREAD_CONDATTR_DEFAULT     nullptr
#endif

#ifndef PTHREAD_SIGACTION
#   define PTHREAD_SIGACTION sigaction
#endif

#ifndef STAT_PARENT
#   define STAT_PARENT                  lstat
#endif

/* socket options which might not be defined on all unx flavors */
#ifndef SO_ACCEPTCONN
#   define SO_ACCEPTCONN    0
#endif
#ifndef SO_SNDLOWAT
#   define SO_SNDLOWAT      0
#endif
#ifndef SO_RCVLOWAT
#   define SO_RCVLOWAT      0
#endif
#ifndef SO_SNDTIMEO
#   define  SO_SNDTIMEO     0
#endif
#ifndef SO_RCVTIMEO
#   define SO_RCVTIMEO      0
#endif
#ifndef SO_USELOOPBACK
#   define SO_USELOOPBACK   0
#endif
#ifndef MSG_MAXIOVLEN
#   define MSG_MAXIOVLEN    0
#endif

/* BEGIN HACK */
/* dummy define and declarations for IPX should be replaced by */
/* original ipx headers when these are available for this platform */

#ifndef SA_FAMILY_DECL
#   define SA_FAMILY_DECL short sa_family
#endif

#define NSPROTO_IPX      1000
#define NSPROTO_SPX      1256
#define NSPROTO_SPXII    1257

/* END HACK */

#ifdef NO_PTHREAD_RTL
#if !defined FREEBSD
#if !defined NETBSD
struct passwd *getpwent_r(struct passwd *pwd, char *buffer,  int buflen);
#endif
extern struct spwd *getspnam_r(const char *name, struct spwd *result,
                               char *buffer, int buflen);

#if !defined MACOSX
struct tm *localtime_r(const time_t *timep, struct tm *buffer);
struct tm *gmtime_r(const time_t *timep, struct tm *buffer);
#endif
#endif /* !defined(FREEBSD) */
#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
