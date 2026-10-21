#ifndef __NET_H
#define __NET_H

#include "main.h"

/********************************** 用户需要设置的参数**********************************/
#define      macUser_ESP8266_ApSsid                       "OPPO Reno10 5G"         		//要连接的热点的名称
#define      macUser_ESP8266_ApPwd                        "12345678"           	//要连接的热点的密钥

#define      macUser_ESP8266_TcpServer_IP                 "192.168.224.20"     //要连接的服务器的 IP
#define      macUser_ESP8266_TcpServer_Port               6666               		//要连接的服务器的端口

/************ 函数声明 ************/
void ESP8266_Task(void);
void ESP8266_TransparentTask(void);
void ESP8266_SendSensorData_Task(void);
#endif /* __APP_ESP8266_H */
