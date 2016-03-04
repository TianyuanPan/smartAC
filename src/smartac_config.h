/*
 * smartac_config.h
 *
 *  Created on: Jan 15, 2016
 *      Author: TianyuanPan
 */

#ifndef SMARTAC_CONFIG_H_
#define SMARTAC_CONFIG_H_

#define  DEFAULT_DEBUGLEVEL        LOG_INFO
#define  DEFAULT_SYSLOG_FACILITY   LOG_DAEMON
#define  DEFAULT_LOG_SYSLOG        0

#define  DEFAULT_ARP_TABLE         "/proc/net/arp"

#define  DEFAULT_CONFIG_FILE       "/etc/wifismartac.conf"

#define  DEFAULT_WIFIDGO_PATH      "/usr/bin/"

//#define  DEFAULT_GW_AC_IFACE       "eth0.1"

#define  DEFAULT_DAEMON            1
#define  DEFAULT_CHECK_INTERVAL    30

#define  DEFAULT_AC_SERVER_PORT    8080
#define  DEFAULT_AC_SERVER_PING_PATH    "/smartac/ping?"
#define  DEFAULT_AC_SERVER_RESULT_PATH  "/smartac/result?"


extern  pthread_mutex_t  config_mutex;


/**
 * Information about the AC server
 */
typedef struct _ac_server_t {
    char *ac_server_hostname;    /**< @brief Hostname of the AC server */
    char *ac_server_ping_path;        /**< @brief Path where to ping */
    char *ac_server_result_path;        /**< @brief Path where to post result data */
    int  ac_server_port;         /**< @brief Http port the AC server listens on */
    char *last_ip;               /**< @brief Last ip used by authserver */
    struct _ac_server_t *next;
}t_ac_server;


/**
 * Popular Servers
 */
typedef struct _popular_server_t {
    char *hostname;
    struct _popular_server_t *next;
} t_popular_server;

/**
 * Configuration structure
 */
typedef struct {
	char *config_file;
	t_ac_server *ac_servers;   /**< @brief AC servers list */
	int check_interval;       /**< @brief How many CheckIntervals ping the AC server */
    char *arp_table_path;     /**< @brief Path to custom ARP table, formatted
                                    like /proc/net/arp */
    char *gw_ac_id;
    char *gw_ac_interface;
    char *gw_ac_ip_address;
    char *gw_ac_mac_address;
    int  is_have_wifidog;
    char *wifidog_path;
    int  daemon;
    int  debuglevel;
    int  syslog_facility;
    t_popular_server *popular_servers; /**< @brief list of popular servers */

}s_config;


/** @brief Get the current gateway configuration */
s_config *config_get_config(void);

/** @brief Initialise the conf system */
void config_init(void);

/** @brief Initialize the variables we override with the command line*/
void config_init_override(void);

/** @brief Reads the configuration file */
void config_read(const char *filename);

/** @brief Check that the configuration is valid */
void config_validate(void);

/** @brief Get the active auth server */
t_ac_server *get_ac_server(void);

/** @brief Bump server to bottom of the list */
void mark_ac_server_bad(t_ac_server *);



#define LOCK_CONFIG() do { \
	debug(LOG_DEBUG, "Locking config"); \
	pthread_mutex_lock(&config_mutex); \
	debug(LOG_DEBUG, "Config locked"); \
} while (0)


#define UNLOCK_CONFIG() do { \
	debug(LOG_DEBUG, "Unlocking config"); \
	pthread_mutex_unlock(&config_mutex); \
	debug(LOG_DEBUG, "Config unlocked"); \
} while (0)

#endif /* SMARTAC_CONFIG_H_ */
