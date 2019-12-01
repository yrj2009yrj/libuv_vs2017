#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <uv.h>

void on_send(uv_udp_send_t *req, int status) {
    if (status) {
        fprintf(stderr, "Send error %s\n", uv_strerror(status));
        return;
    }
}

int main() {
    uv_loop_t *loop = uv_default_loop();
    uv_udp_t *send_socket = (uv_udp_t *)malloc(sizeof(uv_udp_t));
    uv_udp_send_t *send_req = (uv_udp_send_t *)malloc(sizeof(uv_udp_send_t));

    uv_udp_init(loop, send_socket);

    uv_buf_t msgs[10];
    char *ss = (char *)malloc(1024);
    strcpy_s(ss, 1024, "this is a test\n");
    msgs[0] = uv_buf_init(ss, strlen(ss));
    msgs[1] = uv_buf_init(ss, strlen(ss));

    struct sockaddr_in send_addr;
    uv_ip4_addr("192.168.3.55", 6789, &send_addr);
    uv_udp_send(send_req, send_socket, msgs, 2, (const struct sockaddr *)&send_addr, on_send);

    int ret = uv_run(loop, UV_RUN_DEFAULT);

    return ret;
}
