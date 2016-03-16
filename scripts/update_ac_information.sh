#!/bin/sh
#########################################
#
# 作 者：潘田园
# 日 期：2016-03-07
# 版 本：
#
# 脚本描述：
#	此脚本用于统计客户端和AP出口的流量和速率。
#
#################################################

################################################
#
# $(1) operation_flag 
# $(2) ac_version
#
# 参数说明：
#	$1: 操作类型字符串
#	$2: ac 版本字符串
#
################################################


UP_SPEED=/tmp/.client.up.speed      
DOWN_SPEED=/tmp/.client.down.speed  
TRAFFIC=/tmp/.client.traffic
MAC_IP=/tmp/.mac-ip.client
TMP=/tmp/.tmp_data
TRAFFIC_INFO=/tmp/.client.traffic.info
DEVICE_INFO=/tmp/.device.info

OPRATOR_FLAG=$1
AC_VERSION=$2

GW_NETWORK_INTERFACE=$(uci get network.wan.ifname)


###############################################
# 检查防火墙的统计链，
###############################################
chain_check()
{
   upload=$(iptables -w -nvx -L FORWARD | grep UPLOAD)
   download=$(iptables -w -nvx -L FORWARD | grep DOWNLOAD)
   
   if [ -z "$upload" ]
   then
     iptables -w -N UPLOAD
   fi
   
   if [ -z "$download" ]
   then
     iptables -w -N DOWNLOAD
   fi
}

#####################################
# 清除防火墙的统计链
#####################################
chain_clean_up()
{
     iptables -w -nvx -L FORWARD | grep DOWNLOAD | awk '{print $9}' > $MAC_IP
     
     while read line
     do
        iptables -w -D FORWARD -s $line -j UPLOAD
        iptables -w -D FORWARD -d $line -j DOWNLOAD
     done < $MAC_IP
     
     iptables -w -X UPLOAD
     iptables -w -X DOWNLOAD
}


#################################
# 更新防火墙统计链
#################################
update_fw_count_rule()
{
   cat /proc/net/arp | grep : | grep -v 00:00:00:00:00:00 | awk '{print $1}' > $MAC_IP
   while read line
   do
   item=$(iptables -w -nvx -L FORWARD | grep DOWNLOAD | awk '{print $9}' | grep $line)
   if [ -z "$item" ] 
   then
      iptables -w -I FORWARD 1 -s $line -j UPLOAD
      iptables -w -I FORWARD 1 -d $line -j DOWNLOAD
   fi
   done < $MAC_IP
   
   iptables -w -nvx -L FORWARD | grep DOWNLOAD | awk '{print $9}' > $MAC_IP
   
   while read line
   do
   item=$(cat /proc/net/arp | grep : | grep -v 00:00:00:00:00:00 | awk '{print $1}'| grep $line)
   if [ -z "$item" ] 
   then
      iptables -w -D FORWARD -s $line -j UPLOAD
      iptables -w -D FORWARD -d $line -j DOWNLOAD
   fi
   done < $MAC_IP
   
}


#####################################
# 生成客户端流量统计结果文件
#####################################
generate_client_traffic_file()
{
   cat /dev/null > $TRAFFIC
   
   iptables -w -nvx -L FORWARD | grep DOWNLOAD | awk '{print $9,$2}' > $TMP
   while read line
   do
     upload=$(iptables -w -nvx -L FORWARD | grep UPLOAD | grep $(echo $line | awk '{print $1}') | awk '{print $2}')
     echo "$line  $upload" >> $TRAFFIC
   done < $TMP
}


#########################################
# 生成客户端速率结果文件
#########################################
generate_client_rate_file()
{
    tmp1=/tmp/.tmp_data1
    tmp2=/tmp/.tmp_data2
    
    touch $tmp1 $tmp2 
    cat /dev/null > $DOWN_SPEED
    cat /dev/null > $UP_SPEED
    
    iptables -w -nvx -L FORWARD | grep DOWNLOAD | awk '{print $2,$9" down-1"}' >  $TMP   
    iptables -w -nvx -L FORWARD | grep UPLOAD   | awk '{print $2,$8" up-1"}'   >> $TMP 
    sleep 1
    iptables -w -nvx -L FORWARD | grep DOWNLOAD | awk '{print $2,$9" down-2"}' >> $TMP   
    iptables -w -nvx -L FORWARD | grep UPLOAD   | awk '{print $2,$8" up-2"}'   >> $TMP
    
    sort -n -k2 $TMP | grep "down-1" > $tmp1
    sort -n -k2 $TMP | grep "down-2" > $tmp2
    
    while read line
    do
      ip=$(echo $line | awk '{print $2}')
      a=$(echo $line | awk '{print $1}')
      b=$(cat $tmp1 | grep $ip | awk '{print $1}')
      deta=$(($a - $b))
      #echo "DOWN: a=$a b=$b ip=$ip $deta "
      echo "$ip $deta" >> $DOWN_SPEED
   done < $tmp2
   
    sort -n -k2 $TMP | grep "up-1" > $tmp1
    sort -n -k2 $TMP | grep "up-2" > $tmp2
    
    while read line
    do
      ip=$(echo $line | awk '{print $2}')
      a=$(echo $line | awk '{print $1}')
      b=$(cat $tmp1 | grep $ip | awk '{print $1}')
      deta=$(($a - $b))
      #echo "UP: a=$a b=$b ip=$ip $deta "
      echo "$ip $deta" >> $UP_SPEED
   done < $tmp2
    
    rm -rf $tmp1 $tmp2
}

############################################################
# 生成客户端流量信息统计结果文件
#
# 文件的格式是：
#
#  IP地址 上传流量 下载流量 上传速率 下载速率
# ---------------------------------------------------
# 192.168.10.223 45678545 4549814 256 54
# 192.168.10.127 45648464987 5641546584 0 458
#
#############################################################
generate_client_traffic_infomatrion_file()
{
  cat /dev/null > $TMP
   while read line
   do
     
     ip=$(echo $line | awk '{print $1}')
     down_rate=$(cat $DOWN_SPEED | grep $ip | awk '{print $2,$3}')
     up_rate=$(cat $UP_SPEED | grep $ip | awk '{print $2,$3}')
     echo "$line $down_rate $up_rate" >> $TMP
     
   done < $TRAFFIC
   
   cat $TMP | sed 's/[ ][ ]*/ /g' > $TRAFFIC_INFO
}


#########################################
# 更新设备信息的json格式信息
###########################################
update_device_info()
{
   cat /dev/null > $TMP
   echo "\"deviceData\":{" >> $TMP
   echo "\"sys_uptime\":$(cat /proc/uptime|awk -F "." '{print $1}')," >> $TMP
   echo "\"sys_memfree\":$(cat /proc/meminfo|sed -n '2p'|awk '{print $2}')," >> $TMP
   echo "\"sys_load\":$(cat /proc/loadavg | awk '{print $1}')," >> $TMP
   echo "\"gw_mac\":\"$(ifconfig|grep -e $GW_NETWORK_INTERFACE|awk '{print $NF}')\"," >> $TMP
   echo "\"gw_ssid\":\"$(uci get wireless.@wifi-iface[0].ssid)\"," >> $TMP
   echo "\"cpu_use\":$((100-$(top -n1 | sed -n '2p'|awk '{print$8}'|awk -F "%" '{print $1}')))," >> $TMP
   echo "\"ac_version\":\"$AC_VERSION\"," >> $TMP
   echo "\"wan_ip\":\"$(uci get network.wan.ipaddr)\"," >> $TMP
   incoming=$(cat /proc/net/dev|grep $GW_NETWORK_INTERFACE | awk '{print $2}')
   outgoing=$(cat /proc/net/dev|grep $GW_NETWORK_INTERFACE | awk '{print $10}')
   echo "\"incoming\":$incoming," >> $TMP
   echo "\"outgoing\":$outgoing," >> $TMP
   sleep 1
   echo "\"go_speed\":$(($(cat /proc/net/dev|grep $GW_NETWORK_INTERFACE | awk '{print $10}')-$outgoing))," >> $TMP
   echo "\"come_speed\":$(($(cat /proc/net/dev|grep $GW_NETWORK_INTERFACE | awk '{print $2}')-$incoming))}" >> $TMP
   cat $TMP | tr -d '\n' > $DEVICE_INFO
   
}



#########################################
# 数据更新操作函数
########################################
update_operator()
{
     update_fw_count_rule
     generate_client_traffic_file
     generate_client_rate_file
     generate_client_traffic_infomatrion_file
     update_device_info
}

#########################################
# 主函数
###########################################
main_operator()
{
  chain_check
  
  case "$OPRATOR_FLAG" in
      "init_chain") ;;
      "chain_clean_up") chain_clean_up &> /dev/null;;
      "traffic_update") update_operator &> /dev/null;;
      *) return 1;;
  esac
   
  return 0
}


main_operator $OPRATOR_FLAG

echo $?




