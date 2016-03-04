/*
 * smartac_safe.h
 *
 *  Created on: Jan 16, 2016
 *      Author: TianyuanPan
 */

#ifndef SMARTAC_SAFE_H_
#define SMARTAC_SAFE_H_

#include <stdarg.h>             /* For va_list */
#include <sys/types.h>          /* For fork */
#include <unistd.h>             /* For fork */

/** Register an fd for auto-cleanup on fork() */
void register_fd_cleanup_on_fork(const int);

/** @brief Safe version of malloc */
void *safe_malloc(size_t);

/** @brief Safe version of realloc */
void *safe_realloc(void *, size_t);

/* @brief Safe version of strdup */
char *safe_strdup(const char *);

/* @brief Safe version of asprintf */
int safe_asprintf(char **, const char *, ...);

/* @brief Safe version of vasprintf */
int safe_vasprintf(char **, const char *, va_list);

/* @brief Safe version of fork */
pid_t safe_fork(void);

#endif /* SMARTAC_SAFE_H_ */
