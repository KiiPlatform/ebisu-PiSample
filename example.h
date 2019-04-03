#ifndef __example
#define __example

#ifdef __cplusplus
extern 'C' {
#endif

/* Go to https:/developer.kii.com and create app for you! */
const char KII_APP_ID[] = "rr7oqyvzaonp";
/* JP: "api-jp.kii.com" */
/* US: "api.kii.com" */
/* SG: "api-sg.kii.com" */
/* CN: "api-cn3.kii.com" */
const char KII_APP_HOST[] = "api-jp.kii.com";

#define HANDLER_HTTP_BUFF_SIZE 1024
#define HANDLER_MQTT_BUFF_SIZE 1024
#define HANDLER_KEEP_ALIVE_SEC 300

#define UPDATER_HTTP_BUFF_SIZE 1024
#define UPDATE_PERIOD_SEC 60

#define TO_RECV_SEC 15
#define TO_SEND_SEC 15


#ifdef __cplusplus
}
#endif

#endif
