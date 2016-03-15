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

/*
 * @brief: 初始化连接客户端列表函数,通过读取目标文件获取客户端信息构造链表。
 * @client_list *list: 指向构造的链表指针
 * @const char *dhcp_leases_file: 需要读取的目标文件指针
 * */
int init_client_list(client_list *list, const char *dhcp_leases_file);

/*
 * @brief: 释放init_client_list函数分配的内存空间
 * @client_list *list: 指向需要释放的内存地址指针
 * */
void  destory_client_list(client_list *list);


client_list *find_client_by_ip(client_list *list, const char *ip);

client_list *find_client_by_mac(client_list *list, const char *mac);

client_list *find_client_by_hostname(client_list *list, const char *hostname);

client_list *find_client_by_mac_ip(client_list *list, const char *mac, const char *ip);


/*
 * @brief: 初始化客户端传输信息列表函数，通过读取目标文件获取客户端传输信息构造链表。
 * @traffic_list *list:	指向构造的链表指针
 * @const char *traffic_file:	需要读取的目标文件指针
 * */
int init_traffic_list(traffic_list *list, const char *traffic_file);

/*
 * @brief:	通过ip地址查找客户端传输信息链表
 * @traffic_list *list:	指向目标客户端传输信息链表
 * @const char *ip:	指向需要查找的ip地址
 * */
traffic_list *find_traffic_by_ip(traffic_list *list, const char *ip);

/*
 * @brief: 释放init_traffic_list函数分配的内存空间
 * @traffic_list *list: 指向需要释放的内存地址指针
 * */
void  destory_traffic_list(traffic_list *list);

/*
 * @brief:	将客户端传输信息链表与客服端信息链表结合
 * @client_list *c_list:	指向被结合的客户端信息链表
 * @traffic_list *t_list:	指向需要结合的客户端传输信息链表
 * */
void  get_traffic_to_client(client_list *c_list, traffic_list *t_list);

/*
 * @brief:	构造客户端信息json字符串
 * @char *json:	指向构造json字符串指针
 * @client_list *c_list:	指向构造客户端信息json字符串所需的客户端构造链表指针
 * */
int  get_client_list_json(char *json, client_list *c_list);

/*
 * @brief:	构造设备信息json字符串
 * @char *json2:	指向构造的设备信息json字符串指针
 * const char *device_info_file: 指向存储设备信息的文件指针
 * */
int  get_device_info_json(char *json, const char *device_info_file);


int  get_dog_json_info(char *json, const char *wdctl);


int  build_ping_json_data(char *json, const char *gw_id, client_list *c_list);


int update_ac_information(const char *opt_type);


#endif /* SMARTAC_JSON_UTIL_H_ */
