/*
 * server.h
 *
 *  Created on: 7 lis 2020
 *      Author: kuba
 */

#ifndef MAIN_SERVER_H_
#define MAIN_SERVER_H_

#include <esp_http_server.h>

httpd_handle_t start_webserver(void);
void stop_webserver(httpd_handle_t server);

#endif /* MAIN_SERVER_H_ */
