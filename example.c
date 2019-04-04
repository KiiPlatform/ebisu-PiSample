#include "example.h"

#include <tio.h>
#include <jkii.h>

#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>

#include <pthread.h>
#include <unistd.h>
#include "sys_cb_impl.h"
#include "pi_control.h"
#include <stdatomic.h>
#include <signal.h>
#include <stdbool.h>

typedef struct prv_air_conditioner_t {
    kii_bool_t power;
    int temperature;
} prv_air_conditioner_t;

static prv_air_conditioner_t m_air_conditioner;
static pthread_mutex_t m_mutex;

static tio_bool_t prv_get_air_conditioner_info(
        prv_air_conditioner_t* air_conditioner)
{
    if (pthread_mutex_lock(&m_mutex) != 0) {
        return KII_FALSE;
    }
    air_conditioner->power = m_air_conditioner.power;
    int temp = readDS18B20Temparature();
    if (temp <= -9996) {
        printf("failed to read temperature, code: %d\n", temp);
        return KII_FALSE;
    }
    air_conditioner->temperature = temp/1000;
    if (pthread_mutex_unlock(&m_mutex) != 0) {
        return KII_FALSE;
    }
    return KII_TRUE;
}

static kii_bool_t prv_set_air_conditioner_info(
        const prv_air_conditioner_t* air_conditioner)
{
    if (pthread_mutex_lock(&m_mutex) != 0) {
        return KII_FALSE;
    }
    m_air_conditioner.power = air_conditioner->power;
    if (pthread_mutex_unlock(&m_mutex) != 0) {
        return KII_FALSE;
    }
    return KII_TRUE;
}


// Using C11 atomic types.
atomic_bool term_flag = false;
atomic_bool handler_terminated = false;
atomic_bool updater_terminated = false;

tio_bool_t _handler_continue(void* task_info, void* userdata) {
    if (term_flag == true) {
        return KII_FALSE;
    } else {
        return KII_TRUE;
    }
}

tio_bool_t _updater_continue(void* task_info, void* userdata) {
    if (term_flag == true) {
        return KII_FALSE;
    } else {
        return KII_TRUE;
    }
}

void _handler_exit(void* task_info, void* userdata) {
    printf("_handler_exit called\n");
    handler_terminated = true;
}

void _updater_exit(void* task_info, void* userdata) {
    printf("_updater_exit called\n");
    updater_terminated = true;
}

void sig_handler(int sig, siginfo_t *info, void *ctx) {
    term_flag = 1;
}

typedef struct {
    size_t max_size;
    size_t read_size;
} updater_context_t;

size_t updater_cb_state_size(void* userdata)
{
    return 1; // so that TIO_CB_READ will be invoked
}

size_t updater_cb_read(
    char *buffer,
    size_t size,
    void *userdata)
{
    prv_air_conditioner_t air_conditioner;
    int length = sizeof(air_conditioner);
    memset(&air_conditioner, 0x00, length);
    if (prv_get_air_conditioner_info(&air_conditioner) == KII_FALSE) {
        printf("fail to lock.\n");
        return 0;
    }

    char state[100];
    sprintf(
        state,
        "{\"AirConditionerAlias\":{\"power\":%s\"currentTemperature\":%d}}",
        (int)air_conditioner.power == (int)JKII_TRUE ? "true," : "false,",
        air_conditioner.temperature);

    updater_context_t* ctx = (updater_context_t*)userdata;
    size_t read_size = sprintf(buffer, "%.*s", size, &state[ctx->read_size]);
    ctx->read_size += read_size;
    return read_size;
}

tio_bool_t pushed_message_callback(
    const char* message,
    size_t message_length,
    void* userdata)
{
    printf("pushed_message_callback called,\n");
    printf("%.*s\n", (int)message_length, message);
    return KII_FALSE;
}

void handler_init(
        tio_handler_t* handler,
        char* http_buffer,
        int http_buffer_size,
        void* http_ssl_ctx,
        char* mqtt_buffer,
        int mqtt_buffer_size,
        void* mqtt_ssl_ctx,
        jkii_resource_t* resource)
{
    tio_handler_init(handler);

    tio_handler_set_app(handler, KII_APP_ID, KII_APP_HOST);

    tio_handler_set_cb_push(handler, pushed_message_callback, NULL);

    tio_handler_set_cb_task_create(handler, task_create_cb_impl, NULL);
    tio_handler_set_cb_delay_ms(handler, delay_ms_cb_impl, NULL);

    tio_handler_set_cb_sock_connect_http(handler, sock_cb_connect, http_ssl_ctx);
    tio_handler_set_cb_sock_send_http(handler, sock_cb_send, http_ssl_ctx);
    tio_handler_set_cb_sock_recv_http(handler, sock_cb_recv, http_ssl_ctx);
    tio_handler_set_cb_sock_close_http(handler, sock_cb_close, http_ssl_ctx);

    tio_handler_set_cb_sock_connect_mqtt(handler, sock_cb_connect, mqtt_ssl_ctx);
    tio_handler_set_cb_sock_send_mqtt(handler, sock_cb_send, mqtt_ssl_ctx);
    tio_handler_set_cb_sock_recv_mqtt(handler, sock_cb_recv, mqtt_ssl_ctx);
    tio_handler_set_cb_sock_close_mqtt(handler, sock_cb_close, mqtt_ssl_ctx);

    tio_handler_set_mqtt_to_sock_recv(handler, TO_RECV_SEC);
    tio_handler_set_mqtt_to_sock_send(handler, TO_SEND_SEC);

    tio_handler_set_http_buff(handler, http_buffer, http_buffer_size);
    tio_handler_set_mqtt_buff(handler, mqtt_buffer, mqtt_buffer_size);

    tio_handler_set_keep_alive_interval(handler, HANDLER_KEEP_ALIVE_SEC);

    tio_handler_set_json_parser_resource(handler, resource);

    tio_handler_set_cb_task_continue(handler, _handler_continue, NULL);
    tio_handler_set_cb_task_exit(handler, _handler_exit, NULL);
}

void updater_init(
        tio_updater_t* updater,
        char* buffer,
        int buffer_size,
        void* sock_ssl_ctx,
        jkii_resource_t* resource)
{
    tio_updater_init(updater);

    tio_updater_set_app(updater, KII_APP_ID, KII_APP_HOST);

    tio_updater_set_cb_task_create(updater, task_create_cb_impl, NULL);
    tio_updater_set_cb_delay_ms(updater, delay_ms_cb_impl, NULL);

    tio_updater_set_buff(updater, buffer, buffer_size);

    tio_updater_set_cb_sock_connect(updater, sock_cb_connect, sock_ssl_ctx);
    tio_updater_set_cb_sock_send(updater, sock_cb_send, sock_ssl_ctx);
    tio_updater_set_cb_sock_recv(updater, sock_cb_recv, sock_ssl_ctx);
    tio_updater_set_cb_sock_close(updater, sock_cb_close, sock_ssl_ctx);

    tio_updater_set_interval(updater, UPDATE_PERIOD_SEC);

    tio_updater_set_json_parser_resource(updater, resource);

    tio_updater_set_cb_task_continue(updater, _updater_continue, NULL);
    tio_updater_set_cb_task_exit(updater, _updater_exit, NULL);
}

static tio_bool_t tio_action_handler(
    tio_action_t* action,
    tio_action_err_t* error,
    tio_action_result_data_t* data,
    void* userdata)
{
    prv_air_conditioner_t air_conditioner;
    char alias[action->alias_length+1];
    char action_name[action->action_name_length + 1];
    memset(alias, 0, sizeof(alias));
    memset(action_name, 0, sizeof(action_name));
    strncpy(alias, action->alias, action->alias_length);
    strncpy(action_name, action->action_name, action->action_name_length);
    printf("%s: %s\n", alias, action_name);

    if (strcmp(alias, "AirConditionerAlias") != 0) {
        strcpy(error->err_message, "invalid alias");
        return KII_FALSE;
    }

    memset(&air_conditioner, 0, sizeof(air_conditioner));
    if (prv_get_air_conditioner_info(&air_conditioner) == KII_FALSE) {
        printf("fail to lock.\n");
        strcpy(error->err_message, "fail to lock.");
        return KII_FALSE;
    }
    if (strcmp(action_name, "turnPower") == 0) {
        if (action->action_value.type == TIO_TYPE_BOOLEAN) {
            air_conditioner.power = action->action_value.param.bool_value;
            if (air_conditioner.power == KII_TRUE) {
                turnOnLED(0, 50, 0);
            } else {
                turnOffLED();
            }
        } else {
            printf("invalid value.");
            strcpy(error->err_message, "invalid value");
            return KII_FALSE;
        }

    }

    if (prv_set_air_conditioner_info(&air_conditioner) == KII_FALSE) {
        printf("fail to unlock.\n");
        return KII_FALSE;
    }
    return KII_TRUE;
}

static void print_help() {
    printf("sub commands: [onboard|update]\n\n");
    printf("to see detail usage of sub command, execute ./exampleapp {subcommand} --help\n\n");

    printf("onboard with vendor-thing-id\n");
    printf("./exampleapp onboard --vendor-thing-id={vendor thing id} --password={password}\n\n");
}

int main(int argc, char** argv)
{
    // setting up wiringPi
    initLEDPins();

    char* subc = argv[1];

    // Setup Signal handler. (Ctrl-C)
    struct sigaction sa_sigint;
    memset(&sa_sigint, 0, sizeof(sa_sigint));
    sa_sigint.sa_sigaction = sig_handler;
    sa_sigint.sa_flags = SA_SIGINFO;

    if (sigaction(SIGINT, &sa_sigint, NULL) < 0) {
        printf("failed to register sigaction\n");
        exit(1);
    }

    tio_updater_t updater;
    updater_context_t updater_ctx;

    socket_context_t updater_http_ctx;
    updater_http_ctx.to_recv = TO_RECV_SEC;
    updater_http_ctx.to_send = TO_SEND_SEC;

    jkii_token_t updater_tokens[256];
    jkii_resource_t updater_resource = {updater_tokens, 256};

    char updater_buff[UPDATER_HTTP_BUFF_SIZE];
    memset(updater_buff, 0x00, sizeof(char) * UPDATER_HTTP_BUFF_SIZE);
    updater_init(
            &updater,
            updater_buff,
            UPDATER_HTTP_BUFF_SIZE,
            &updater_http_ctx,
            &updater_resource);

    tio_handler_t handler;

    socket_context_t handler_http_ctx;
    handler_http_ctx.to_recv = TO_RECV_SEC;
    handler_http_ctx.to_send = TO_SEND_SEC;

    socket_context_t handler_mqtt_ctx;
    handler_mqtt_ctx.to_recv = TO_RECV_SEC;
    handler_mqtt_ctx.to_send = TO_SEND_SEC;

    char handler_http_buff[HANDLER_HTTP_BUFF_SIZE];
    memset(handler_http_buff, 0x00, sizeof(char) * HANDLER_HTTP_BUFF_SIZE);

    char handler_mqtt_buff[HANDLER_MQTT_BUFF_SIZE];
    memset(handler_mqtt_buff, 0x00, sizeof(char) * HANDLER_MQTT_BUFF_SIZE);

    jkii_token_t handler_tokens[256];
    jkii_resource_t handler_resource = {handler_tokens, 256};

    handler_init(
            &handler,
            handler_http_buff,
            HANDLER_HTTP_BUFF_SIZE,
            &handler_http_ctx,
            handler_mqtt_buff,
            HANDLER_MQTT_BUFF_SIZE,
            &handler_mqtt_ctx,
            &handler_resource);

    if (argc < 2) {
        printf("too few arguments.\n");
        print_help();
        exit(1);
    }

    /* Parse command. */
    if (strcmp(subc, "onboard") == 0) {
        char* vendorThingID = NULL;
        char* password = NULL;
        while(1) {
            struct option longOptions[] = {
                {"vendor-thing-id", required_argument, 0, 0},
                {"password", required_argument, 0, 1},
                {"help", no_argument, 0, 2},
                {0, 0, 0, 0}
            };
            int optIndex = 0;
            int c = getopt_long(argc, argv, "", longOptions, &optIndex);
            const char* optName = longOptions[optIndex].name;
            if (c == -1) {
                if (vendorThingID == NULL) {
                    printf("neither vendor-thing-id is specified.\n");
                    exit(1);
                }
                if (password == NULL) {
                    printf("password is not specifeid.\n");
                    exit(1);
                }
                tio_code_t result = tio_handler_onboard(
                        &handler,
                        vendorThingID,
                        password,
                        NULL,
                        NULL,
                        NULL,
                        NULL);
                if (result != TIO_ERR_OK) {
                    printf("failed to onboard.\n");
                    exit(1);
                }
                printf("Onboarding succeeded!\n");
                break;
            }
            printf("option %s : %s\n", optName, optarg);
            switch(c) {
                case 0:
                    vendorThingID = optarg;
                    break;
                case 1:
                    password = optarg;
                    break;
                case 2:
                    printf("usage: \n");
                    printf("onboard --vendor-thing-id={ID of the thing} --password={password of the thing}\n");
                    break;
                default:
                    printf("unexpected usage.\n");
            }
            if (strcmp(optName, "help") == 0) {
                exit(0);
            }
        }
    } else {
        print_help();
        exit(0);
    }

    const kii_author_t* author = tio_handler_get_author(&handler);
    tio_handler_start(&handler, author, tio_action_handler, NULL);
    tio_updater_start(
            &updater,
            author,
            updater_cb_state_size,
            &updater_ctx,
            updater_cb_read,
            &updater_ctx);

    bool end = false;
    bool disp_msg = false;
    while(!end){
        sleep(1);
        if (term_flag && !disp_msg) {
            printf("Waiting for exiting tasks...\n");
            disp_msg = true;
        }
        if (handler_terminated && updater_terminated) {
            end = true;
        }
    };
}

/* vim: set ts=4 sts=4 sw=4 et fenc=utf-8 ff=unix: */