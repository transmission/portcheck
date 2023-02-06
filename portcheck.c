#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h> /* Required by event.h */
#include <sys/queue.h> /* Required by event.h */
#include <evhttp.h>

static void SignalCallback(int s, short event, void *arg);
static void Handler(struct evhttp_request *req, void *arg);
static void SendReply(struct evhttp_request *req, int i_code);
static void WriteCallback(int s, short event, void *_req);

int
main(int i_argc, char **pp_argv)
{
    if (i_argc != 2) {
        return 1;
    }
    /* Get which port we should be listening on */
    long i_port = strtol(pp_argv[1], NULL, 10);
    if (i_port < 1 || i_port > 65535) {
        return 1;
    }

    event_init();

    struct event evSignal;
    event_set(&evSignal, SIGINT, EV_SIGNAL | EV_PERSIST, SignalCallback, NULL);
    event_add(&evSignal, NULL);

    struct evhttp *httpd = evhttp_start("0.0.0.0", i_port);
    if (httpd == NULL) {
        return 1;
    }
    evhttp_set_gencb(httpd, Handler, NULL);

    event_dispatch();

    evhttp_free(httpd);

    return 0;
}

static void
SignalCallback(int s, short event, void *arg)
{
    event_loopexit(NULL);
}

static void
Handler(struct evhttp_request *req, void *arg)
{
    /* Get port from URI */
    const char *p_uri = evhttp_request_uri(req);
    if (p_uri == NULL) {
        goto error;
    }
    char *p_end;
    long i_port = strtol(p_uri + 1, &p_end, 10);
    if (p_end == p_uri + 1 || *p_end != '\0') {
        goto error;
    }
    if (i_port < 1 || i_port > 65535) {
        goto error;
    }

    /* Get IP from X-Forwarded-For. This code relies on the port checker
     * being run behind a proxy that properly sets it, such as lighttpd */
    const char *p_ip =
      evhttp_find_header(req->input_headers, "X-Forwarded-For");
    if (p_ip == NULL) {
        goto error;
    }
    if (strncmp(p_ip, "::ffff:", 7) == 0) {
        /* IPv4-mapped IPv6 address */
        p_ip += 7;
    }
    if (inet_addr(p_ip) == INADDR_NONE) {
        goto error;
    }

    /* We have a proper IP and port, try to connect to it */
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        goto error;
    }
    if (fcntl(s, F_SETFL, O_NONBLOCK) != 0) {
        close(s);
        goto error;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(p_ip);
    addr.sin_port = htons(i_port);
    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
        /* Immediate success */
        close(s);
        SendReply(req, 1);
    } else if (errno != EINPROGRESS) {
        /* Immediate failure */
        close(s);
        SendReply(req, 0);
    } else {
        /* Give it 5 seconds to connect */
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        event_once(s, EV_WRITE, WriteCallback, req, &tv);
    }
    return;

error:
    evhttp_send_reply(req, HTTP_BADREQUEST, "Bad Request", NULL);
}

static void
SendReply(struct evhttp_request *req, int i_code)
{
    struct evbuffer *buf = evbuffer_new();
    evbuffer_add_printf(buf, "%d", i_code);
    evhttp_send_reply(req, HTTP_OK, "OK", buf);
    evbuffer_free(buf);
}

static void
WriteCallback(int s, short event, void *_req)
{
    struct evhttp_request *req = _req;

    int error;
    socklen_t errsz = sizeof(error);
    if (event != EV_TIMEOUT &&
      getsockopt(s, SOL_SOCKET, SO_ERROR, &error, &errsz) == 0 &&
      error == 0) {
        SendReply(req, 1);
    } else {
        SendReply(req, 0);
    }
    close(s);
}
