/*
 * smartac.c
 *
 *  Created on: Jan 15, 2016
 *      Author: TianyuanPan
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>

#include <pthread.h>
#include <signal.h>
#include <errno.h>

/* for strerror() */
#include <string.h>

/* for wait() */
#include <sys/wait.h>

#include "smartac_commandline.h"
#include "smartac_config.h"
#include "smartac_debug.h"
#include "smartac_ping.h"
#include "smartac_post_result.h"
#include "smartac_resultqueue.h"
#include "smartac_json_util.h"

#include "smartac.h"


t_queue cmdrets_queue;

static pthread_t tid_ping = 0;

time_t started_time = 0;




/**@internal
 * @brief Handles SIGCHLD signals to avoid zombie processes
 *
 * When a child process exits, it causes a SIGCHLD to be sent to the
 * process. This handler catches it and reaps the child process so it
 * can exit. Otherwise we'd get zombie processes.
 */
void sigchld_handler(int s)
{
    int status;
    pid_t rc;

    debug(LOG_DEBUG, "Handler for SIGCHLD called. Trying to reap a child");

    rc = waitpid(-1, &status, WNOHANG);

    debug(LOG_DEBUG, "Handler for SIGCHLD reaped child PID %d", rc);
}


/** Exits cleanly after cleaning up the firewall.
 *  Use this function anytime you need to exit after firewall initialization.
 *  @param s Integer that is really a boolean, true means voluntary exit, 0 means error.
 */
void termination_handler(int s)
{
    static pthread_mutex_t sigterm_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_t self = pthread_self();

    debug(LOG_INFO, "Handler for termination caught signal %d", s);

    /* Makes sure we only call fw_destroy() once. */
    if (pthread_mutex_trylock(&sigterm_mutex)) {
        debug(LOG_INFO, "Another thread already began global termination handler. I'm exiting");
        pthread_exit(NULL);
    } else {
        debug(LOG_INFO, "Cleaning up and exiting");
    }

    /* XXX Hack
     * Aparently pthread_cond_timedwait under openwrt prevents signals (and therefore
     * termination handler) from happening so we need to explicitly kill the threads
     * that use that
     */
    if (tid_ping && self != tid_ping) {
        debug(LOG_INFO, "Explicitly killing the ping thread");
        pthread_kill(tid_ping, SIGKILL);
    }

    destroy_queue(&cmdrets_queue);

    if (update_ac_information(opt_type[OPT_T_CHAIN_CLEAN_UP]) != 0){
    	debug(LOG_ERR, "at main_loop, update_ac_information(opt_type[OPT_T_CHAIN_CLEAN_UP]) error!");
    }

    debug(LOG_NOTICE, "Exiting...");
    exit(s == 0 ? 1 : 0);
}


/** @internal
 * Registers all the signal handlers
 */
static void init_signals(void)
{
    struct sigaction sa;

    debug(LOG_DEBUG, "Initializing signal handlers");

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        debug(LOG_ERR, "sigaction(): %s", strerror(errno));
        exit(1);
    }

    /* Trap SIGPIPE */
    /* This is done so that when libhttpd does a socket operation on
     * a disconnected socket (i.e.: Broken Pipes) we catch the signal
     * and do nothing. The alternative is to exit. SIGPIPE are harmless
     * if not desirable.
     */
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) == -1) {
        debug(LOG_ERR, "sigaction(): %s", strerror(errno));
        exit(1);
    }

    sa.sa_handler = termination_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    /* Trap SIGTERM */
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        debug(LOG_ERR, "sigaction(): %s", strerror(errno));
        exit(1);
    }

    /* Trap SIGQUIT */
    if (sigaction(SIGQUIT, &sa, NULL) == -1) {
        debug(LOG_ERR, "sigaction(): %s", strerror(errno));
        exit(1);
    }

    /* Trap SIGINT */
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        debug(LOG_ERR, "sigaction(): %s", strerror(errno));
        exit(1);
    }
}

/**@internal
 * Main execution loop
 */
static void  main_loop(void)
{
	int result;
	pthread_t tid_post;
	s_config *config = config_get_config();
	void **params;

	t_result *cmdrets = NULL;

    /* Set the time when AC started */
    if (!started_time) {
        debug(LOG_INFO, "Setting started_time");
        started_time = time(NULL);
    } else if (started_time < MINIMUM_STARTED_TIME) {
        debug(LOG_WARNING, "Detected possible clock skew - re-setting started_time");
        started_time = time(NULL);
    }

    /* if we don't have the Gateway internet interface, get it. Can't fail. */
    if(!config->gw_ac_interface){
        debug(LOG_DEBUG, "Finding Gateway external Interface ...");
        if ((config->gw_ac_interface = get_ext_iface()) == NULL) {
            debug(LOG_ERR, "Could not get Gateway Interface information, exiting...");
            exit(1);
        }
        debug(LOG_DEBUG, "Get the GW Interface of:%s", config->gw_ac_interface);
    }

    /* If we don't have the Gateway IP address, get it. */
    if (!config->gw_ac_ip_address) {
        debug(LOG_DEBUG, "Finding IP address of: %s", config->gw_ac_interface);
        if ((config->gw_ac_ip_address = get_iface_ip(config->gw_ac_interface)) == NULL) {
            debug(LOG_ERR, "Could not get IP address information of: %s ", config->gw_ac_ip_address);
        }
        debug(LOG_DEBUG, "%s = %s", config->gw_ac_ip_address, config->gw_ac_ip_address);
    }

    /* If we don't have the Gateway ID, construct it from the internal MAC address.
     * "Can't fail" so exit() if the impossible happens. */
    if (!config->gw_ac_mac_address) {
        debug(LOG_DEBUG, "Finding MAC address of: %s", config->gw_ac_interface);
        if ((config->gw_ac_mac_address = get_iface_mac(config->gw_ac_interface)) == NULL) {
            debug(LOG_ERR, "Could not get MAC address information of: %s, exiting...", config->gw_ac_interface);
            exit(1);
        }
        debug(LOG_DEBUG, "Get Interface %s MAC addr %s", config->gw_ac_interface, config->gw_ac_mac_address);
        if(!config->gw_ac_id){
        	config->gw_ac_id = safe_strdup(config->gw_ac_mac_address);
            debug(LOG_DEBUG, "%s = %s", config->gw_ac_id, config->gw_ac_mac_address);
        }
    }

	/* set excute out dir, can't fail */
	if (init_excute_outdir() < 0){
		debug(LOG_ERR, "FATAL: Failed to initalize excute out directory.");
		exit(1);
	}

	/* initial command execute result queue, can't fail */
	if (initial_queue(&cmdrets_queue) != 0){
		debug(LOG_ERR, "FATAL: Failed to initalize command result queue.");
		exit(1);
	}


    /* Init the signals to catch chld/quit/etc */
    init_signals();


    /* Start heartbeat thread */
    result = pthread_create(&tid_ping, NULL, (void *)thread_ping, NULL);
    if (result != 0) {
        debug(LOG_ERR, "FATAL: Failed to create a new thread (ping) - exiting");
        termination_handler(0);
    }
    pthread_detach(tid_ping);


    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;
    struct timespec timeout;

    /* this is post settings to server */
    char cmd[64] = {0};
    sprintf(cmd, "%s 0 %s","accmd_getsettings", config->gw_ac_id);
    if (excute_remote_shell_command(config->gw_ac_id, cmd) !=0 )
    	debug(LOG_WARNING, "Warning: Failed to execute get settings command.");;

    while(1){

        if (!config->gw_ac_ip_address) {
            debug(LOG_DEBUG, "Finding IP address of: %s", config->gw_ac_interface);
            if ((config->gw_ac_ip_address = get_iface_ip(config->gw_ac_interface)) == NULL) {
                debug(LOG_ERR, "Could not get IP address information of: %s ", config->gw_ac_ip_address);
            }
            debug(LOG_DEBUG, "%s = %s", config->gw_ac_ip_address, config->gw_ac_ip_address);
        }


    	cmdrets = getout_queue(&cmdrets_queue);

    	if (cmdrets){
           if (pthread_create(&tid_post, NULL, (void *)thread_post_result, cmdrets) != 0){
                debug(LOG_WARNING, "FATAL: Failed to create a new thread_post_result - exiting");
                cmdresult_free(cmdrets);
           }else
            	pthread_detach(tid_post);
    	}

        /* Sleep for 5 seconds... */
        timeout.tv_sec = time(NULL) + 5L;
        timeout.tv_nsec = 0;

        /* Mutex must be locked for pthread_cond_timedwait... */
        pthread_mutex_lock(&cond_mutex);

        /* Thread safe "sleep" */
        pthread_cond_timedwait(&cond, &cond_mutex, &timeout);

        /* No longer needs to be locked */
        pthread_mutex_unlock(&cond_mutex);
    }
}




int smartac_main(int argc, char **argv)
{
    s_config *config = config_get_config();

    config_init();

    parse_commandline(argc, argv);

    /* Initialize the config */
    config_read(config->config_file);
    config_validate();


    if (restart_orig_pid) {

        /*
         * At this point the parent will start destroying itself and the firewall. Let it finish it's job before we continue
         */
        while (kill(restart_orig_pid, 0) != -1) {
            debug(LOG_INFO, "Waiting for parent PID %d to die before continuing loading", restart_orig_pid);
            sleep(1);
        }

        debug(LOG_INFO, "Parent PID %d seems to be dead. Continuing loading.");
    }

    if (config->daemon) {

        debug(LOG_INFO, "Forking into background");

        switch (safe_fork()) {
        case 0:                /* child */
            setsid();
            main_loop();
            break;

        default:               /* parent */
            exit(0);
            break;
        }
    } else {
        main_loop();
    }

	return (0);
}


