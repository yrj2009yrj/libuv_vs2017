/*
 *  ����Tcp��������˳��
 *  1����ʼ��uv_tcp_t: uv_tcp_init(loop, &tcp_server)
 *  2���󶨵�ַ��uv_tcp_bind
 *  3���������ӣ�uv_listen
 *  4��ÿ����һ�����ӽ���֮�󣬵���uv_listen�Ļص����ص���Ҫ���������飺
 *    4.1����ʼ���ͻ��˵�tcp�����uv_tcp_init()
 *    4.2�����ոÿͻ��˵����ӣ�uv_accept()
 *    4.3����ʼ��ȡ�ͻ�����������ݣ�uv_read_start()
 *    4.4����ȡ����֮������Ӧ�����������Ҫ��Ӧ�ͻ������ݣ�����uv_write����д���ݼ��ɡ�
 * ��������֪ʶ�㣬��demo���õ���timer�����
 */

#include <stdio.h>
#include "uv.h"
#include "common.h"

#define HOST "0.0.0.0"
#define PORT 9999

// ��̬tcp���
static uv_tcp_t tcp_server_handle;

void close_cb(uv_handle_t *client) {
    // �ͷ����tcp_client_handle
    free(client);
    printf("connection closed\n");
}

void shutdown_cb(uv_shutdown_t *req, int status) {
    // �ر����shutdown_req
    uv_close((uv_handle_t *)req->handle, close_cb);
    free(req);
}

void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
    if (buf->base == NULL) {
        printf("alloc_cb malloc buffer error\n");
    }
}

void write_cb(uv_write_t* req, int status) {
    CHECK(status, "write_cb");
    // �ͷŵ�����֮ǰ�����uv_write_req

    //printf("server had reponsed\n");

    write_req_t *write_req = (write_req_t *)req;

    // ���ﲻ����Ҫ�����ͷţ���Ϊ�����Buf����malloc��
    // free(write_req->buf.base);
    free(write_req);
}

void write_to_client(char *resp, uv_stream_t* stream) {
    int r = 0;
    write_req_t * write_req = malloc(sizeof(write_req_t));

    write_req->buf = uv_buf_init(resp, strlen(resp));
    r = uv_write(&write_req->req, stream, &write_req->buf, 1, write_cb);
    CHECK(r, "uv_write");
}

void read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    int r = 0;
    // �����ݰ�����Ҫ�����ж�nread����ֶ�
    if (nread < 0) {
        if (nread != UV_EOF) {
            CHECK(nread, "read_cb");
        }

        free(buf->base);

        // ��ȡ���ݵ���β�ˣ��ͻ���û��������Ҫ������
        uv_shutdown_t *shutdown_req = malloc(sizeof(uv_shutdown_t));
        r = uv_shutdown(shutdown_req, stream, shutdown_cb);
        CHECK(r, "uv_shutdown");
        return;
    }

    if (nread == 0) {
        free(buf->base);
        return;
    }

    (buf->base)[nread] = '\0';
    printf("===>:%s\n", buf->base);
    // ������ȡ���ݣ��ж������ǲ���������Ҫ�ģ����ǵĻ��ͷ��ش������Ϣ��֪�ͻ���
    if (!strncmp("Hello", buf->base, strlen(buf->base))) {
        write_to_client("world", stream);
    }
    else if (!strncmp("Libuv", buf->base, strlen(buf->base))) {
        write_to_client("I love", stream);
    }
    else {
        write_to_client("Unknown argot", stream);
    }
}

void connection_cb(uv_stream_t *server, int status) {
    int r = 0;
    // ��ʼ���ͻ��˵�tcp���
    uv_tcp_t *tcp_client_handle = malloc(sizeof(uv_tcp_t));
    r = uv_tcp_init(server->loop, tcp_client_handle);

    // �����������
    r = uv_accept(server, (uv_stream_t *)tcp_client_handle);

    printf("A client has connected to me\n");

    if (r < 0) {
        // �����������ʧ�ܣ���Ҫ����һЩ����
        uv_shutdown_t *shutdown_req = malloc(sizeof(uv_shutdown_t));

        r = uv_shutdown(shutdown_req, (uv_stream_t *)tcp_client_handle, shutdown_cb);
        CHECK(r, "uv_shutdown");
    }

    // ���ӽ��ܳɹ�֮�󣬿�ʼ��ȡ�ͻ��˴��������
    // ���ｫuv_tcp_t����uv_pipe_tҲ��û����ģ������Ļ�����ʹ��uv_pipe_init����ʼ����
    r = uv_read_start((uv_stream_t *)tcp_client_handle, alloc_cb, read_cb);
}

void timer_cb(uv_timer_t *handle) {
    uv_print_active_handles(handle->loop, stderr);
    printf("loop is alive[%d], timer handle is active[%d], now[%lld], hrtime[%lld]\n",
        uv_loop_alive(handle->loop), uv_is_active((uv_handle_t *)handle), uv_now(handle->loop), uv_hrtime());
}

int main() {
    uv_loop_t *loop = uv_default_loop();
    int r = 0;

    // ��ʼ��tcp��������ﲻ�������κ�socket
    r = uv_tcp_init(loop, &tcp_server_handle);
    CHECK(r, "uv_tcp_init");

    // ��ʼ����ƽ̨���õ�ipv4��ַ
    struct sockaddr_in addr;
    r = uv_ip4_addr(HOST, PORT, &addr);
    CHECK(r, "uv_ipv4_addr");

    // ��
    r = uv_tcp_bind(&tcp_server_handle, (struct sockaddr *) &addr, AF_INET);
    CHECK(r, "uv_tcp_bind");

    // ��ʼ��������
    r = uv_listen((uv_stream_t *)&tcp_server_handle, SOMAXCONN, connection_cb);
    CHECK(r, "uv_listen");

    // The stdout file handle (which printf writes to) is by default line buffered.
    // That means output is buffered until there is a newline, when the buffer is flushed.
    // That's why you should always end your output with a newline.
    // ��������������printf��ӡ�󲻼�\n�Ļ������еĴ�ӡ���������һ��ֱ����\n
    printf("tcp server listen at %s:%d\n", HOST, PORT);


    // ����һ����ʱ��ȥѯ�ʵ�ǰ�ǲ���һֱ�л�Ծ�ľ�����Դ�����֤ĳЩ�۵�
    uv_timer_t timer_handle;
    r = uv_timer_init(loop, &timer_handle);
    CHECK(r, "uv_timer_init");

    // ÿ10���ӵ��ö�ʱ���ص�һ��
    r = uv_timer_start(&timer_handle, timer_cb, 10 * 1000, 10 * 1000);

    return uv_run(loop, UV_RUN_DEFAULT);
}
