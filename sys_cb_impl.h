#ifndef __SOCK_CB_LINUX
#define __SOCK_CB_LINUX

#include <khc_socket_callback.h>
#include <openssl/ssl.h>
#include <kii_task_callback.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    SSL *ssl;
    SSL_CTX *ssl_ctx;
    int socket;
    unsigned int to_recv;
    unsigned int to_send;
} socket_context_t;

khc_sock_code_t
    sock_cb_connect(void* sock_ctx, const char* host,
            unsigned int port);

khc_sock_code_t
    sock_cb_send(void* sock_ctx,
            const char* buffer,
            size_t length,
            size_t* out_sent_length);

khc_sock_code_t
    sock_cb_recv(void* sock_ctx, char* buffer, size_t length_to_read,
            size_t* out_actual_length);

khc_sock_code_t
    sock_cb_close(void* sock_context);

/** Implementation of callback to create task.
 * this SDK requirest to implement this function in each
 * target environment.
 *
 * This function is assigned to fields
 * kii_t#kii_core_t#kii_http_context_t#kii_http_context_t#task_create_cb
 * of command handler and state updater.
 *
 * @param [in] name name of task.
 * @param [in] entry entry of task.
 * @param [in] stk_start start position of stack area.
 * @param [in] stk_size stack size of task
 * @param [in] priority priority of thisk
 *
 * @return KII_TASKC_OK if succeed to create task. otherwise KII_TASKC_FAIL.
 */
kii_task_code_t task_create_cb_impl(
        const char* name,
        KII_TASK_ENTRY entry,
        void* param,
        void* userdata);

/** Implementation of callback to delay task.
 * this SDK requirest to implement this function in each
 * target environment.
 *
 * This function is assigned to fields
 * kii_t#kii_core_t#kii_http_context_t#kii_http_context_t#delay_ms_cb
 * of command handler and state updater.
 *
 * @param[in] msec millisecond to delay.
 */
void delay_ms_cb_impl(unsigned int msec, void* userdata);

#ifdef __cplusplus
}
#endif

#endif /* __SOCK_CB_LINUX */
