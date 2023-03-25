#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <err.h>
#include <sys/socket.h>
#include <glib.h>

void rewrite(int fd, const void *buf, size_t count)
{
    int left = count;
    void* p_buf = (char*) buf;
    do {
        int n_written = write(fd, buf, count);
        if (n_written == -1) {
            errx(-1, "write()\n");
        }
        p_buf += n_written;
        left -= n_written;
    } while (left > 0);
}

void printrsc(int fd_in)
{
    char buf[128];
    int n_read = 1;
    GString* req = g_string_new("");
    do {
        n_read = read(fd_in, buf, 128);
	if (n_read == -1) {
            errx(-1, "read()\n");
	}
	g_string_append_len(req, buf, n_read);
    } while (!g_str_has_suffix(req->str, "\r\n\r\n") && (n_read > 0));
    gchar* p_rsc = NULL;
    gsize rsc_len = 0;
    for (gsize i = 0; i < req->len; i++) {
        if (p_rsc == NULL) {	
            if (req->str[i] == '/') {
                p_rsc = &(req->str[i]);
		p_rsc += 1; // ignore '/' character
	    }
        }
	else {
            if (req->str[i] == ' ') {
                break;
	    }
            rsc_len += 1;
	}
    }
    gchar* rsc = g_strndup(p_rsc, rsc_len);
    g_print("Resource = %s\n", rsc);
    g_string_free(req, TRUE);
    g_free(rsc);
}

int main()
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* result = NULL;
    int err = getaddrinfo(NULL, "2048", &hints, &result);
    if (err != 0) {
        errx(err, "getaddrinfo(): %s", gai_strerror(err));
    }

    int sfd = -1;
    for (struct addrinfo* p = result; p != NULL; p = p->ai_next) {
        sfd = socket(
            p->ai_family,
            p->ai_socktype,
            p->ai_protocol
        );
        if (sfd == -1) {
            continue;
        }
        int opt_val = 1;
        err = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));
        if (err != 0) {
            errx(err, "setsockopt()");
        }
        if (bind(sfd, p->ai_addr, p->ai_addrlen) == 0) {
            break;
        }
        close(sfd);
    }
    freeaddrinfo(result);
    if (sfd == -1) {
        errx(sfd, "socket(), bind()");
    }

    err = listen(sfd, 5);
        if (err != 0) {
        errx(-1, "listen()");
    }
    printf("Static Server\n");
    printf("Listening to port 2048...\n");
    char response[] = "HTTP/1.1 200 OK\r\n\r\nHello World!";
    while (1) {
        int accepted_sfd = accept(sfd, NULL, NULL);
	if (accepted_sfd == -1) {
            close(sfd);
	    errx(-1, "accept()");
	}
	rewrite(accepted_sfd, response, strlen(response));
	printrsc(accepted_sfd);
        close(accepted_sfd);
    }
    close(sfd);
    return 0;
}

