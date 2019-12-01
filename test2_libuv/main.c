#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <uv.h>

void on_read(uv_fs_t *req);
static char buffer[1024];
static uv_buf_t iov;
int opened_fd = 0;

void on_write(uv_fs_t *req) {
    uv_fs_t *read_req = req->data;

    if (req->result < 0) {
        fprintf(stderr, "Write error: %s\n", uv_strerror((int)req->result));
    }
    else {
        uv_fs_read(uv_default_loop(), read_req, opened_fd, &iov, 1, -1, on_read);
    }
}

void on_read(uv_fs_t *req) {
    uv_fs_t *write_req = req->data;

    if (req->result < 0) {
        fprintf(stderr, "Read error: %s\n", uv_strerror(req->result));
    }
    else if (req->result == 0) {
        uv_fs_req_cleanup(req);
        uv_fs_req_cleanup(write_req);
        free(req);
        free(write_req);
        uv_fs_t close_req;
        // synchronous
        uv_fs_close(uv_default_loop(), &close_req, opened_fd, NULL);
    }
    else if (req->result > 0) {
        iov.len = req->result;
        write_req->data = req;
        uv_fs_write(uv_default_loop(), write_req, 1, &iov, 1, -1, on_write);
    }
}

void on_open(uv_fs_t *req) {
    if (req->result >= 0) {
        uv_fs_t *read_req = malloc(sizeof(uv_fs_t));
        uv_fs_t *write_req = malloc(sizeof(uv_fs_t));
        read_req->data = write_req;
        
        iov = uv_buf_init(buffer, 1024);
        opened_fd = req->result;
        uv_fs_read(uv_default_loop(), read_req, req->result,
            &iov, 1, -1, on_read); 
    }
    else {
        fprintf(stderr, "error opening file: %s\n", uv_strerror((int)req->result));
    }
}

int main(int argc, char **argv) {
    uv_fs_t open_req;
    uv_fs_open(uv_default_loop(), &open_req, argv[1], O_RDONLY, 0, on_open);
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    uv_fs_req_cleanup(&open_req);
    return 0;
}
