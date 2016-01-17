/*
 * smartac_config.c
 *
 *  Created on: Jan 15, 2016
 *      Author: TianyuanPan
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>

#include "smartac_common.h"
#include "smartac_safe.h"
#include "smartac_debug.h"
#include "smartac_http.h"
#include "smartac_util.h"


#include "smartac_config.h"


/** @internal
 * Holds the current configuration of the gateway */
static s_config config;


/**
 * Mutex for the configuration file, used by the auth_servers related
 * functions. */
pthread_mutex_t config_mutex = PTHREAD_MUTEX_INITIALIZER;

/** @internal
 * A flag.  If set to 1, there are missing or empty mandatory parameters in the config
 */
static int missing_parms;

/** @internal
 The different configuration options */
typedef enum {
    oBadOption,
    oGwAcId,
    oDaemon,
    oDebugLevel,
    oArpTable,
    oAcServer,
    oAcServHostname,
    oAcServHttpPort,
    oAcServPingPath,
    oAcServResultPath,
    oCheckInterval,
    oSyslogFacility,
    oPopularServers
} OpCodes;



/** @internal
 The config file keywords for the different configuration options */
static const struct {
    const char *name;
    OpCodes opcode;
} keywords[] = {
    {
    "Daemon", oDaemon}, {
    "DebugLevel", oDebugLevel}, {
    "GatewayAcId", oGwAcId}, {
    "ArpTablePath", oArpTable}, {
    "CheckInterval", oCheckInterval}, {
    "SyslogFacility", oSyslogFacility}, {
    "SmartAcServer", oAcServer}, {
    "Hostname", oAcServHostname}, {
    "PopularServers", oPopularServers}, {
    "HttpPort", oAcServHttpPort}, {
    "PingPath", oAcServPingPath}, {
    "ResultPath", oAcServResultPath}, {
    NULL, oBadOption},
};

static void config_notnull(const void *, const char *);
static int parse_boolean_value(char *);
static void parse_ac_server(FILE *, const char *, int *);
static OpCodes config_parse_token(const char *, const char *, int);
static void parse_popular_servers(const char *);
static void add_popular_server(const char *);
static void validate_popular_servers(void);



/** Accessor for the current gateway configuration
@return:  A pointer to the current config.  The pointer isn't opaque, but should be treated as READ-ONLY
 */
s_config *
config_get_config(void)
{
    return &config;
}

/** Sets the default config parameters and initialises the configuration system */
void
config_init(void)
{
    debug(LOG_DEBUG, "Setting default config parameters");

    config.config_file = safe_strdup(DEFAULT_CONFIG_FILE);
    config.arp_table_path = safe_strdup(DEFAULT_ARP_TABLE);
    config.ac_servers = NULL;
    config.check_interval = DEFAULT_CHECK_INTERVAL;
    config.debuglevel = DEFAULT_DEBUGLEVEL;
    config.daemon = DEFAULT_DAEMON;
    config.gw_ac_id = NULL;
    config.syslog_facility = DEFAULT_SYSLOG_FACILITY;

    debugconf.log_stderr = 1;
    debugconf.debuglevel = DEFAULT_DEBUGLEVEL;
    debugconf.syslog_facility = config.syslog_facility;
    debugconf.log_syslog = DEFAULT_LOG_SYSLOG;
}


/**
 * If the command-line didn't provide a config, use the default.
 */
void
config_init_override(void)
{
    if (config.daemon == -1) {
        config.daemon = DEFAULT_DAEMON;
        if (config.daemon > 0) {
            debugconf.log_stderr = 0;
        }
    }
}


/** @internal
Parses a single token from the config file
*/
static OpCodes
config_parse_token(const char *cp, const char *filename, int linenum)
{
    int i;

    for (i = 0; keywords[i].name; i++)
        if (strcasecmp(cp, keywords[i].name) == 0)
            return keywords[i].opcode;

    debug(LOG_ERR, "%s: line %d: Bad configuration option: %s", filename, linenum, cp);
    return oBadOption;
}



/** @internal
Parses auth server information
*/
static void
parse_ac_server(FILE * file, const char *filename, int *linenum)
{
    char *host = NULL,
         *ping_path = NULL,
         *result_path = NULL,
         line[MAX_BUF], *p1, *p2;

    int http_port, opcode;

    t_ac_server *new, *tmp;

    /* Defaults */
    ping_path = safe_strdup(DEFAULT_AC_SERVER_PING_PATH);
    result_path = safe_strdup(DEFAULT_AC_SERVER_RESULT_PATH);
    http_port = DEFAULT_AC_SERVER_PORT;

    /* Parsing loop */
    while (memset(line, 0, MAX_BUF) && fgets(line, MAX_BUF - 1, file) && (strchr(line, '}') == NULL)) {
        (*linenum)++;           /* increment line counter. */

        /* skip leading blank spaces */
        for (p1 = line; isblank(*p1); p1++) ;

        /* End at end of line */
        if ((p2 = strchr(p1, '#')) != NULL) {
            *p2 = '\0';
        } else if ((p2 = strchr(p1, '\r')) != NULL) {
            *p2 = '\0';
        } else if ((p2 = strchr(p1, '\n')) != NULL) {
            *p2 = '\0';
        }

        /* trim all blanks at the end of the line */
        for (p2 = (p2 != NULL ? p2 - 1 : &line[MAX_BUF - 2]); isblank(*p2) && p2 > p1; p2--) {
            *p2 = '\0';
        }

        /* next, we coopt the parsing of the regular config */
        if (strlen(p1) > 0) {
            p2 = p1;
            /* keep going until word boundary is found. */
            while ((*p2 != '\0') && (!isblank(*p2)))
                p2++;

            /* Terminate first word. */
            *p2 = '\0';
            p2++;

            /* skip all further blanks. */
            while (isblank(*p2))
                p2++;

            /* Get opcode */
            opcode = config_parse_token(p1, filename, *linenum);

            switch (opcode) {
            case oAcServHostname:
                /* Coverity rightfully pointed out we could have duplicates here. */
                if (NULL != host)
                    free(host);
                host = safe_strdup(p2);
                break;
            case oAcServPingPath:
                free(ping_path);
                ping_path = safe_strdup(p2);
                break;
            case oAcServResultPath:
                free(result_path);
                result_path = safe_strdup(p2);
                break;
            case oAcServHttpPort:
                http_port = atoi(p2);
                break;
            case oBadOption:
            default:
                debug(LOG_ERR, "Bad option on line %d " "in %s.", *linenum, filename);
                debug(LOG_ERR, "Exiting...");
                exit(-1);
                break;
            }
        }
    }

    /* only proceed if we have an host and a path */
    if (host == NULL) {
        free(ping_path);
        free(result_path);
        return;
    }

    debug(LOG_DEBUG, "Adding %s:%d  PingPath:%s  ResultPath:%s to the AC server list", host,
    		     http_port, ping_path, result_path);

    /* Allocate memory */
    new = safe_malloc(sizeof(t_ac_server));

    /* Fill in struct */
    new->ac_server_hostname = host;
    new->ac_server_ping_path = ping_path;
    new->ac_server_result_path = result_path;
    new->ac_server_port = http_port;

    /* If it's the first, add to config, else append to last server */
    if (config.ac_servers == NULL) {
        config.ac_servers = new;
    } else {
        for (tmp = config.ac_servers; tmp->next != NULL; tmp = tmp->next) ;
        tmp->next = new;
    }

    debug(LOG_DEBUG, "AC server added");
}



/**
Advance to the next word
@param s string to parse, this is the next_word pointer, the value of s
	 when the macro is called is the current word, after the macro
	 completes, s contains the beginning of the NEXT word, so you
	 need to save s to something else before doing TO_NEXT_WORD
@param e should be 0 when calling TO_NEXT_WORD(), it'll be changed to 1
	 if the end of the string is reached.
*/
#define TO_NEXT_WORD(s, e) do { \
	while (*s != '\0' && !isblank(*s)) { \
		s++; \
	} \
	if (*s != '\0') { \
		*s = '\0'; \
		s++; \
		while (isblank(*s)) \
			s++; \
	} else { \
		e = 1; \
	} \
} while (0)



/**
@param filename Full path of the configuration file to be read
*/
void
config_read(const char *filename)
{
    FILE *fd;
    char line[MAX_BUF], *s, *p1, *p2, *rawarg = NULL;
    int linenum = 0, opcode, value;
    size_t len;

    debug(LOG_INFO, "Reading configuration file '%s'", filename);

    if (!(fd = fopen(filename, "r"))) {
        debug(LOG_ERR, "Could not open configuration file '%s', " "exiting...", filename);
        exit(1);
    }

    while (!feof(fd) && fgets(line, MAX_BUF, fd)) {
        linenum++;
        s = line;

        if (s[strlen(s) - 1] == '\n')
            s[strlen(s) - 1] = '\0';

        if ((p1 = strchr(s, ' '))) {
            p1[0] = '\0';
        } else if ((p1 = strchr(s, '\t'))) {
            p1[0] = '\0';
        }

        if (p1) {
            p1++;

            // Trim leading spaces
            len = strlen(p1);
            while (*p1 && len) {
                if (*p1 == ' ')
                    p1++;
                else
                    break;
                len = strlen(p1);
            }
            rawarg = safe_strdup(p1);
            if ((p2 = strchr(p1, ' '))) {
                p2[0] = '\0';
            } else if ((p2 = strstr(p1, "\r\n"))) {
                p2[0] = '\0';
            } else if ((p2 = strchr(p1, '\n'))) {
                p2[0] = '\0';
            }
        }

        if (p1 && p1[0] != '\0') {
            /* Strip trailing spaces */

            if ((strncmp(s, "#", 1)) != 0) {
                debug(LOG_DEBUG, "Parsing token: %s, " "value: %s", s, p1);
                opcode = config_parse_token(s, filename, linenum);

                switch (opcode) {

                case oDaemon:
                    if (config.daemon == -1 && ((value = parse_boolean_value(p1)) != -1)) {
                        config.daemon = value;
                        if (config.daemon > 0) {
                            debugconf.log_stderr = 0;
                        } else {
                            debugconf.log_stderr = 1;
                        }
                    }
                    break;
                case oGwAcId:
                    config.gw_ac_id = safe_strdup(p1);
                    break;
                case oArpTable:
                	config.arp_table_path = safe_strdup(p1);
                	break;
                case oAcServer:
                    parse_ac_server(fd, filename, &linenum);
                    break;
                case oPopularServers:
                    parse_popular_servers(rawarg);
                    break;
                case oCheckInterval:
                    sscanf(p1, "%d", &config.check_interval);
                    break;
                case oDebugLevel:
                	sscanf(p1, "%d", &config.debuglevel);
                	break;
                case oSyslogFacility:
                    sscanf(p1, "%d", &debugconf.syslog_facility);
                    break;
                case oBadOption:
                    /* FALL THROUGH */
                default:
                    debug(LOG_ERR, "Bad option on line %d " "in %s.", linenum, filename);
                    debug(LOG_ERR, "Exiting...");
                    exit(-1);
                    break;
                }
            }
        }
        if (rawarg) {
            free(rawarg);
            rawarg = NULL;
        }
    }

    fclose(fd);
}


/** @internal
 * Add a popular server to the list. It prepends for simplicity.
 * @param server The hostname to add.
 */
static void
add_popular_server(const char *server)
{
    t_popular_server *p = NULL;

    p = (t_popular_server *)safe_malloc(sizeof(t_popular_server));
    p->hostname = safe_strdup(server);

    if (config.popular_servers == NULL) {
        p->next = NULL;
        config.popular_servers = p;
    } else {
        p->next = config.popular_servers;
        config.popular_servers = p;
    }
}

static void
parse_popular_servers(const char *ptr)
{
    char *ptrcopy = NULL;
    char *hostname = NULL;
    char *tmp = NULL;

    debug(LOG_DEBUG, "Parsing string [%s] for popular servers", ptr);

    /* strsep modifies original, so let's make a copy */
    ptrcopy = safe_strdup(ptr);

    while ((hostname = strsep(&ptrcopy, ","))) {  /* hostname does *not* need allocation. strsep
                                                     provides a pointer in ptrcopy. */
        /* Skip leading spaces. */
        while (*hostname != '\0' && isblank(*hostname)) {
            hostname++;
        }
        if (*hostname == '\0') {  /* Equivalent to strcmp(hostname, "") == 0 */
            continue;
        }
        /* Remove any trailing blanks. */
        tmp = hostname;
        while (*tmp != '\0' && !isblank(*tmp)) {
            tmp++;
        }
        if (*tmp != '\0' && isblank(*tmp)) {
            *tmp = '\0';
        }
        debug(LOG_DEBUG, "Adding Popular Server [%s] to list", hostname);
        add_popular_server(hostname);
    }

    free(ptrcopy);
}

/** @internal
Parses a boolean value from the config file
*/
static int
parse_boolean_value(char *line)
{
    if (strcasecmp(line, "yes") == 0) {
        return 1;
    }
    if (strcasecmp(line, "no") == 0) {
        return 0;
    }
    if (strcmp(line, "1") == 0) {
        return 1;
    }
    if (strcmp(line, "0") == 0) {
        return 0;
    }

    return -1;
}


/** Verifies if the configuration is complete and valid.  Terminates the program if it isn't */
void
config_validate(void)
{
    config_notnull(config.ac_servers, "AuthServer");
    validate_popular_servers();

    if (missing_parms) {
        debug(LOG_ERR, "Configuration is not complete, exiting...");
        exit(-1);
    }
}


/** @internal
 * Validate that popular servers are populated or log a warning and set a default.
 */
static void
validate_popular_servers(void)
{
    if (config.popular_servers == NULL) {
        debug(LOG_WARNING, "PopularServers not set in config file, this will become fatal in a future version.");
        add_popular_server("www.baidu.com");
        add_popular_server("www.hao123.com");
    }
}

/** @internal
    Verifies that a required parameter is not a null pointer
*/
static void
config_notnull(const void *parm, const char *parmname)
{
    if (parm == NULL) {
        debug(LOG_ERR, "%s is not set", parmname);
        missing_parms = 1;
    }
}

/**
 * This function returns the current (first auth_server)
 */
t_ac_server *
get_ac_server(void)
{

    /* This is as good as atomic */
    return config.ac_servers;
}


/**
 * This function marks the current auth_server, if it matches the argument,
 * as bad. Basically, the "bad" server becomes the last one on the list.
 */
void
mark_ac_server_bad(t_ac_server * bad_server)
{
    t_ac_server *tmp;

    if (config.ac_servers == bad_server && bad_server->next != NULL) {
        /* Go to the last */
        for (tmp = config.ac_servers; tmp->next != NULL; tmp = tmp->next) ;
        /* Set bad server as last */
        tmp->next = bad_server;
        /* Remove bad server from start of list */
        config.ac_servers = bad_server->next;
        /* Set the next pointe to NULL in the last element */
        bad_server->next = NULL;
    }

}


