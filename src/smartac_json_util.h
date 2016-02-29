/*
 * smartac_json_util.h
 *
 *  Created on: Feb 22, 2016
 *      Author: TianyuanPan
 */

#ifndef SMARTAC_JSON_UTIL_H_
#define SMARTAC_JSON_UTIL_H_

#define DHCP_LEASES_FILE  "/tmp/dhcp.leases"

#define TRAFFIC_FILE      "/tmp/.client.traffic.info"

#define DEVICE_INFO_FILE   "/tmp/.device.info"

#define MAX_STRING_LEN     1024*10

#define OPT_T_SCRIPT          "/usr/bin/update_ac_information.sh"
#define OPT_T_INIT_CHAIN      0
#define OPT_T_CHAIN_CLEAN_UP  1
#define OPT_T_TRAFFIC_UPDATE  2

struct _client_{

	char c_ip[16];
	char c_mac[18];
	char c_hostname[64];
	long long incoming;
	long long outgoing;
	int go_speed;
	int come_speed;

	struct _client_ *next;

};

typedef struct  _client_ client_list;


struct _traffic_ {

	char traffic_ip[16];
	long long incoming;
	long long outgoing;
	int go_speed;
	int come_speed;

	struct _traffic_ *next;
};

typedef struct _traffic_ traffic_list;


extern const char *opt_type[];
extern client_list   *c_list;
extern traffic_list  *t_list;

int init_client_list(client_list *list, const char *dhcp_leases_file);

void  destory_client_list(client_list *list);

client_list *find_client_by_ip(client_list *list, const char *ip);

client_list *find_client_by_mac(client_list *list, const char *mac);

client_list *find_client_by_hostname(client_list *list, const char *hostname);

client_list *find_client_by_mac_ip(client_list *list, const char *mac, const char *ip);


int init_traffic_list(traffic_list *list, const char *traffic_file);

traffic_list *find_traffic_by_ip(traffic_list *list, const char *ip);

void  destory_traffic_list(traffic_list *list);

void  get_traffic_to_client(client_list *c_list, traffic_list *t_list);


int  get_client_list_json(char *json, client_list *c_list);

int  get_device_info_json(char *json, const char *device_info_file);

int  get_dog_json_info(char *json, const char *wdctl);

int  build_ping_json_data(char *json, const char *gw_id, client_list *c_list);

int update_ac_information(const char *opt_type);

#endif /* SMARTAC_JSON_UTIL_H_ */
