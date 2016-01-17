/*
 * smart_util.h
 *
 *  Created on: Jan 15, 2016
 *      Author: TianyuanPan
 */

#ifndef SMARTAC_UTIL_H_
#define SMARTAC_UTIL_H_

#include <stddef.h>
#include <stdarg.h> /* For va_list */

/** @brief Execute a shell command */
int execute(const char *, int);


/** @brief Thread safe gethostbyname */
struct in_addr *wd_gethostbyname(const char *);

/**
 * Structure to represent a pascal-like string.
 */
struct pstr {
    char *buf;   /**< @brief Buffer used to hold string. Pointer subject to change. */
    size_t len;  /**< @brief Current length of the string. */
    size_t size; /**< @brief Current maximum size of the buffer. */
};

typedef struct pstr pstr_t;  /**< @brief pstr_t is a type for a struct pstr. */

pstr_t *pstr_new(void);  /**< @brief Create a new pstr */
char * pstr_to_string(pstr_t *);  /**< @brief Convert pstr to a char *, freeing pstr. */
void pstr_cat(pstr_t *, const char *);  /**< @brief Appends a string to a pstr_t */
int pstr_append_sprintf(pstr_t *, const char *, ...);  /**< @brief Appends formatted string to a pstr_t. */


#endif /* SMARTAC_UTIL_H_ */
