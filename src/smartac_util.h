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


/** How many times should we try detecting the interface with the default route
 * (in seconds).  If set to 0, it will keep retrying forever */
#define NUM_EXT_INTERFACE_DETECT_RETRY 0
/** How often should we try to detect the interface with the default route
 *  if it isn't up yet (interval in seconds) */
#define EXT_INTERFACE_DETECT_RETRY_INTERVAL 1



/** @brief Execute a shell command */
int execute(const char *, int);


/** @brief Thread safe gethostbyname */
struct in_addr *wd_gethostbyname(const char *);

/** @brief Get IP address of an interface */
char *get_iface_ip(const char *);

/** @brief Get MAC address of an interface */
char *get_iface_mac(const char *);

/** @brief Get interface name of default gateway */
char *get_ext_iface(void);

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
