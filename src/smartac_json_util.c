/*
 * smartac_json_util.c
 *
 *  Created on: Feb 22, 2016
 *      Author: TianyuanPan
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#include "smartac_debug.h"
#include "smartac_config.h"

#include "smartac_json_util.h"

/*
 * @brief client_list 和 traffic_list 的全局指针
 */
client_list   *c_list;
traffic_list  *t_list;


/* @brief 流量更新统计的操作类型 */
const char *opt_type[] = {
		{"init_chain"},
		{"chain_clean_up"},
		{"traffic_update"}
};

/*
 * @brief: 初始化连接客户端列表函数,通过读取目标文件获取客户端信息构造链表。
 * @client_list *list: 指向构造的链表指针
 * @const char *dhcp_leases_file: 需要读取的目标文件指针
 * */
int init_client_list(client_list *list, const char *dhcp_leases_file)
{
	FILE *fp;
	client_list *head, *pre;
	char *seg[16], line[512], *ptr;
	int i;
	char delimiter = ' ';

	if (!list)
		return -1;

	fp = fopen(dhcp_leases_file, "r");

	if (!fp){
		debug(LOG_ERR, "at init_client_list(...), fopen(...) error.");
		return -1;
	}

	head = list;
	pre  = list;
	head->next = NULL;
	while ( fgets(line, 512, fp) > 0 ){
		ptr = line;
		i = 0;

		while ((seg[i] = strsep(&ptr, &delimiter)) != NULL) i++;

		if (!head){

			head = (client_list *)malloc(sizeof(client_list));
			if(!head){
				fclose(fp);
				destory_client_list(list);
				debug(LOG_ERR, "at init_client_list(...), malloc error.");
				return -1;
			}
			pre->next = head;
			pre = head;
		}

		if (seg[1])
		    sprintf(head->c_mac, "%s", seg[1]);
		else
			memset(head->c_mac, 0, 18);
		if (seg[2])
		    sprintf(head->c_ip, "%s", seg[2]);
		else
			memset(head->c_ip, 0, 16);
		if (seg[3])
		    sprintf(head->c_hostname, "%s", seg[3]);
		else
			memset(head->c_hostname, 0, 64);
		head->come_speed = 0;
		head->go_speed = 0;
		head->incoming = 0;
		head->outgoing = 0;
		head->next = NULL;
		head = head->next;

		memset(line, 0, 512);/**/
	}

	fclose(fp);

	return 0;
}


/*
 * @brief: 释放init_client_list函数分配的内存空间
 * @client_list *list: 指向需要释放的内存地址指针
 * */
void  destory_client_list(client_list *list)
{
	client_list *p1,*p2;
	p1 = list;

	while (p1){
		p2 = p1->next;
		free(p1);
		p1 = p2;
	}

	list = NULL;
}


client_list *find_client_by_ip(client_list *list, const char *ip)
{
	client_list *ptr = NULL;

	ptr = list;

	while (ptr){

		if ( 0 == strcmp(ptr->c_ip, ip) )
			return ptr;

		ptr = ptr->next;
	}

	return NULL;
}



client_list *find_client_by_mac(client_list *list, const char *mac)
{
	client_list *ptr = NULL;

	ptr = list;

	while (ptr){

		if ( 0 == strcmp(ptr->c_mac, mac) )
			return ptr;

		ptr = ptr->next;
	}

	return NULL;
}



client_list *find_client_by_hostname(client_list *list, const char *hostname)
{
	client_list *ptr = NULL;

	ptr = list;

	while (ptr){

		if ( 0 == strcmp(ptr->c_hostname, hostname) )
			return ptr;

		ptr = ptr->next;
	}

	return NULL;
}



client_list *find_client_by_mac_ip(client_list *list, const char *mac, const char *ip)
{
	client_list *ptr = NULL;

	ptr = list;

	while (ptr){

		if ( 0 == strcmp(ptr->c_mac, mac) && 0 == strcmp(ptr->c_ip, ip))
			return ptr;

		ptr = ptr->next;
	}

	return NULL;
}



/*
 * @brief: 初始化客户端传输信息列表函数，通过读取目标文件获取客户端传输信息构造链表。
 * @traffic_list *list:	指向构造的链表指针
 * @const char *traffic_file:	需要读取的目标文件指针
 * */
int init_traffic_list(traffic_list *list, const char *traffic_file)
{
	FILE *fp;
	traffic_list *head, *pre;
	char *seg[16], line[1024], *ptr;
	int i;

	char delimiter = ' ';

	if (!list)
		return -1;

	fp = fopen(traffic_file, "r");

	if (!fp){
		debug(LOG_ERR, "at init_traffic_list(...), fopen(...) error.");
		return -1;
	}

	head = list;
	pre  = list;
	head->next = NULL;
	while ( fgets(line, 1024, fp) > 0 ){
		ptr = line;
		i = 0;

		while ((seg[i] = strsep(&ptr, &delimiter)) != NULL) i++;

		if (!head){

			head = (traffic_list *)malloc(sizeof(traffic_list));
			if(!head){
				fclose(fp);
				destory_traffic_list(list);
				debug(LOG_ERR, "at init_traffic_list(...), malloc error.");
				return -1;
			}
			pre->next = head;
			pre = head;
		}

		if (seg[0])
		   sprintf(head->traffic_ip, "%s", seg[0]);
		if (seg[1])
		   head->incoming = atoll(seg[1]);
		else head->incoming = 0;
		if (seg[2])
		   head->outgoing = atoll(seg[2]);
		else head->outgoing = 0;
		if (seg[3])
		   head->come_speed = atoi(seg[3]);
		else head->come_speed = 0;
		if (seg[4])
		   head->go_speed = atoi(seg[4]);
		else head->go_speed = 0;
		head->next = NULL;
		head = head->next;

		memset(line, 0, 1024);/**/
	}

	fclose(fp);
	return 0;
}


/*
 * @brief:	通过ip地址查找客户端传输信息链表
 * @traffic_list *list:	指向目标客户端传输信息链表
 * @const char *ip:	指向需要查找的ip地址
 * */
traffic_list *find_traffic_by_ip(traffic_list *list, const char *ip)
{
	traffic_list *ptr = NULL;

	ptr = list;

	while (ptr){

		if ( 0 == strcmp(ptr->traffic_ip, ip) )
			return ptr;

		ptr = ptr->next;
	}
	return NULL;
}


/*
 * @brief: 释放init_traffic_list函数分配的内存空间
 * @traffic_list *list: 指向需要释放的内存地址指针
 * */
void  destory_traffic_list(traffic_list *list)
{
	traffic_list *p1,*p2;
	p1 = list;

	while (p1){
		p2 = p1->next;
		free(p1);
		p1 = p2;
	}

	list = NULL;
}


/*
 * @brief:	将客户端传输信息链表与客服端信息链表结合
 * @client_list *c_list:	指向被结合的客户端信息链表
 * @traffic_list *t_list:	指向需要结合的客户端传输信息链表
 * */
void  get_traffic_to_client(client_list *c_list, traffic_list *t_list)
{
	client_list  *c_p;
	traffic_list *t_p;

	if (!c_list || !t_list)
		return;

	c_p = c_list;

	while (c_p){

		t_p = find_traffic_by_ip(t_list, c_p->c_ip);

		if (!t_p){
			c_p = c_p->next;
			continue;
		}

		c_p->incoming = t_p->incoming;
		c_p->outgoing = t_p->outgoing;
		c_p->go_speed = t_p->go_speed;
		c_p->come_speed = t_p->come_speed;

		c_p = c_p->next;
	}

	return;
}

/*
 * @brief:	构造客户端信息json字符串
 * @char *json:	指向构造json字符串指针
 * @client_list *c_list:指向构造客户端信息json字符串所需的客户端构造链表指针
 * */
int  get_client_list_json(char *json, client_list *c_list)
{
	char tmp[MAX_STRING_LEN] = {0},
	     *ptr;
	client_list *c_p;
	int cur_len = 0;

	if (!json || !c_list)
		return -1;

	ptr = tmp;

	sprintf(ptr, "%s", "\"clientList\":[");

	c_p = c_list;

	while (c_p){

		cur_len = strlen(tmp);
		ptr = tmp + cur_len;

		sprintf(ptr, "{\"mac\":\"%s\",\"ip\":\"%s\",\"incoming\":%lld,"
				"\"outgoing\":%lld,\"hostname\":\"%s\",\"go_speed\":%d,\"come_speed\":%d},",
				c_p->c_mac, c_p->c_ip, c_p->incoming,
				c_p->outgoing, c_p->c_hostname, c_p->go_speed, c_p->come_speed);

		c_p = c_p->next;
	}

	cur_len = strlen(tmp);
	tmp[cur_len - 1] = ' ';
	tmp[cur_len] = ']';

	sprintf(json, "%s", tmp);

	return 0;
}


/*
 * @brief:	构造设备信息json字符串
 * @char *json2:	指向构造的设备信息json字符串指针
 * const char *device_info_file: 指向存储设备信息的文件指针
 * */
int  get_device_info_json(char *json, const char *device_info_file)
{
     FILE *fp;

     struct stat stat_buf;

     int file_len;

     char tmp[MAX_STRING_LEN] = {0};

     if (!json)
    	 return -1;

    if ( stat(device_info_file, &stat_buf) != 0)
    	return -1;

    file_len = stat_buf.st_size;

    if (file_len > MAX_STRING_LEN){
    	return -1;
    }

     fp = fopen(device_info_file, "r");

     if (!fp)
    	 return -1;

     fread(tmp, 1, file_len, fp);

     fclose(fp);

     sprintf(json, "%s", tmp);

     return 0;
}



int  get_dog_json_info(char *json, const char *wdctl)
{
	char *tmp[MAX_STRING_LEN] = {0};
	FILE *fp;

	if (!json){
		return -1;
	}

	fp = popen(wdctl, "r");

	if (!fp){
		debug(LOG_ERR, "at get_dog_json_info(...), popen(...) error.");
		return -1;
	}

	fread(tmp, 1, MAX_STRING_LEN, fp);

	pclose(fp);

	sprintf(json, "\"wifidog_status\":%s", tmp);

	return 0;
}



int  build_ping_json_data(char *json, const char *gw_id, client_list *c_list)
{

	char tmp[MAX_STRING_LEN] = {0},
		 tmp2[MAX_STRING_LEN] = {0},
		 tmpdog[MAX_STRING_LEN] = {0},
		 *ptr;
    int  cur_len;

    char wdctl[128];
    s_config *config = config_get_config();

    if (!json){
    	return -1;
    }

    sprintf(tmp, "{\"gw_id\":\"%s\",", gw_id);

    ptr =  tmp + strlen(tmp);



    if (config->is_have_wifidog)
       sprintf(ptr, "\"wifidog_flag\":true,");
    else
    	sprintf(ptr, "\"wifidog_flag\":false,");

    ptr =  tmp + strlen(tmp);

    if ( get_device_info_json(tmp2, DEVICE_INFO_FILE) != 0) {
    	debug(LOG_ERR, "at build_ping_json_data(...), get_device_info_json(...) error.");
    	return -1;
    }

    sprintf(ptr, "%s,", tmp2);

    ptr =  tmp + strlen(tmp);

    memset(tmp2, 0, MAX_STRING_LEN);

    if ( get_client_list_json(tmp2, c_list) != 0) {
    	debug(LOG_ERR, "at build_ping_json_data(...), get_client_list_json(...) error.");
    	return -1;
    }

    sprintf(ptr, "%s", tmp2);

    ptr =  tmp + strlen(tmp);

    memset(tmp2, 0, MAX_STRING_LEN);

    if (config->is_have_wifidog){
    	sprintf(wdctl, "%swdctl status", config->wifidog_path);
    	if (get_dog_json_info(tmpdog, wdctl) != 0){
    		sprintf(ptr, ",\"wifidog_status\":%s", "{}}");
    		debug(LOG_ERR, "at build_ping_json_data(...), get_dog_json_info(...) error.");
    		goto out;
    	}
        sprintf(ptr, ",%s}", tmpdog);
    }else
    	sprintf(ptr, "%s", "}");

out:
    sprintf(json, "%s", tmp);

	return 0;
}


int update_ac_information(const char *opt_type)
{
	FILE *fp;
	char ret[10],
	     cmd[128];

	sprintf(cmd, "%s  %s", OPT_T_SCRIPT, opt_type);

	fp = popen(cmd, "r");

	if (!fp){
		debug(LOG_ERR, "at update_ac_information(...), popen(...) error.");
		return -1;
	}

	fread(ret, 1, 10, fp);

	pclose(fp);

	return (atoi(ret) == 0) ? 0 : -1;

}
