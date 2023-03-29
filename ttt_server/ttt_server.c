#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <err.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <glib.h>

// Grid of the game.
// "_" represents an empty cell.
// "x" represents the cross.
// "o" represents the nought.
char grid[] = "_________";

// Current number of players.
int player_count = 0;

pthread_mutex_t lock;

void rewrite(int fd, const void *buf, size_t count)
{
    int left = count;
    void *p_buf = (char *)buf;
    do
    {
        int n_written = write(fd, buf, count);
        if (n_written == -1)
        {
            errx(-1, "write()\n");
        }
        p_buf += n_written;
        left -= n_written;
    } while (left > 0);
}

void resend(int fd, const void *buf, size_t count)
{
    int flag = count == 0 ? 0 : MSG_MORE;
    if (flag == 0)
    {
        // Explicitly make an end singal.
        int err = send(fd, "", count, flag);
        if (err == -1)
        {
            errx(-1, "send()\n");
        }
        return;
    }
    int left = count;
    void *p_buf = (char *)buf;
    do
    {
        int n_written = send(fd, buf, count, flag);
        if (n_written == -1)
        {
            errx(-1, "send()\n");
        }
        p_buf += n_written;
        left -= n_written;
    } while (left > 0);
}

void echo(int fd_in, int fd_out)
{
    char buf[128];
    int n_read = 1;
    do
    {
        n_read = read(fd_in, buf, 128);
        if (n_read == -1)
        {
            errx(-1, "read()");
        }
        resend(fd_out, buf, n_read);
    } while (n_read > 0);
}

void echo_format(int fd_in, int fd_out, const gchar *name, const gchar ox)
{
    char buf[128];
    int n_read = 1;
    GString *html = g_string_new("");
    do
    {
        n_read = read(fd_in, buf, 128);
        if (n_read == -1)
        {
            errx(-1, "read()");
        }
        g_string_append_len(html, buf, n_read);
    } while (n_read > 0);
    gchar *html_formatted = g_strdup_printf(html->str, ox, name);
    rewrite(fd_out, html_formatted, strlen(html_formatted));
    g_string_free(html, TRUE);
    g_free(html_formatted);
}

gchar *extractpath(int fd_in, gboolean print_stdout)
{
    char buf[128];
    int n_read = 1;
    GString *req = g_string_new("");
    do
    {
        n_read = read(fd_in, buf, 128);
        if (n_read == -1)
        {
            errx(-1, "read()\n");
        }
        g_string_append_len(req, buf, n_read);
    } while (!g_str_has_suffix(req->str, "\r\n\r\n") && (n_read > 0));
    gchar *p_rsc = NULL;
    gsize rsc_len = 0;
    for (gsize i = 0; i < req->len; i++)
    {
        if (p_rsc == NULL)
        {
            if (req->str[i] == '/')
            {
                p_rsc = &(req->str[i]);
                p_rsc += 1; // ignore '/' character
            }
        }
        else
        {
            if (req->str[i] == ' ')
            {
                break;
            }
            rsc_len += 1;
        }
    }
    gchar *rsc = g_strndup(p_rsc, rsc_len);
    if (print_stdout)
    {
        g_print("%d: ", fd_in);
        if (strlen(rsc))
        {
            g_print("%s\n", rsc);
        }
        else
        {
            g_print("index.html\n");
        }
    }
    g_string_free(req, TRUE);
    return rsc;
}

gchar *extractnickname(const gchar *cmd)
{
    gchar key[] = "nickname=";
    size_t i;
    gchar *p_cmd = (gchar *)cmd;
    for (i = 0; i < strlen(cmd); i++)
    {
        if (strncmp(p_cmd, key, strlen(key)) == 0)
        {
            break;
        }
        p_cmd += 1;
    }
    gchar *nickname = g_strndup(p_cmd + strlen(key), strlen(cmd) - (i + strlen(key)));
    return nickname;
}

gchar *getfname(const gchar *rsc)
{
    gchar *ret;
    int err;
    if (strlen(rsc) == 0)
    {
        err = asprintf(&ret, "www/%s", "index.html");
    }
    else
    {
        err = asprintf(&ret, "www/%s", rsc);
    }
    if (err < 0)
    {
        errx(-1, "asprintf()");
    }
    return ret;
}

void sendres(const gchar *path, int accepted_sfd)
{
    gchar protocol[] = "HTTP/1.1 ";
    resend(accepted_sfd, protocol, strlen(protocol));
    gchar ok[] = "200 OK\r\n\r\n";
    // gchar notfound[] = "404 Not Found\r\n\r\n";
    gchar *fpath = getfname(path);
    int fd = open(fpath, O_RDONLY);
    if (fd != -1) // Resource
    {
        resend(accepted_sfd, ok, strlen(ok));
        echo(fd, accepted_sfd);
    }
    else // Command
    {
        int player_count_local;
        gchar *nickname = NULL;
        if (g_str_has_prefix(path, "update"))
        {
            resend(accepted_sfd, ok, strlen(ok));
            rewrite(accepted_sfd, grid, strlen(grid));
        }
        else if (g_str_has_prefix(path, "set"))
        {
            resend(accepted_sfd, ok, strlen(ok));
            char turn = path[4];
            int cell = path[5] - '0';
            grid[cell] = turn;
            rewrite(accepted_sfd, grid, strlen(grid));
        }
        else if (g_str_has_prefix(path, "restart"))
        {
            resend(accepted_sfd, ok, strlen(ok));
            for (size_t i = 0; i < strlen(grid); i++)
            {
                grid[i] = '_';
            }
            rewrite(accepted_sfd, grid, strlen(grid));
        }
        else if (g_str_has_prefix(path, "grid"))
        {
            pthread_mutex_lock(&lock);
            player_count_local = player_count;
            pthread_mutex_unlock(&lock);
            char symbol;
            if (player_count_local == 0)
            {
                symbol = 'x';
            }
            if (player_count_local == 1)
            {
                symbol = 'o';
            }
            if (player_count_local < 2)
            {
                pthread_mutex_lock(&lock);
                // printf("player_count: %d\n", player_count);
                player_count++;
                pthread_mutex_unlock(&lock);
                fpath = getfname("grid.html");
                fd = open(fpath, O_RDONLY);
                nickname = extractnickname(path);
                resend(accepted_sfd, ok, strlen(ok));
                echo_format(fd, accepted_sfd, nickname, symbol);
                g_free(nickname);
            }
            else
            {
                fpath = getfname("busy.html");
                fd = open(fpath, O_RDONLY);
                resend(accepted_sfd, ok, strlen(ok));
                echo(fd, accepted_sfd);
            }
        }
        else
        {
            pthread_mutex_lock(&lock);
            player_count_local = player_count;
            pthread_mutex_unlock(&lock);
            if (player_count_local < 2)
            {
                fpath = getfname("new_player.html");
            }
            else
            {
                fpath = getfname("busy.html");
            }
            fd = open(fpath, O_RDONLY);
            resend(accepted_sfd, ok, strlen(ok));
            echo(fd, accepted_sfd);
        }
    }
    g_free(fpath);
    close(fd);
}

void *worker(void *arg)
{
    int accepted_sfd = *((int *)arg);
    free(arg);

    gchar *path = extractpath(accepted_sfd, TRUE);
    sendres(path, accepted_sfd);

    close(accepted_sfd);
    g_free(path);

    return NULL;
}

int main()
{
    pthread_mutex_init(&lock, NULL);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *result = NULL;
    int err = getaddrinfo(NULL, "2048", &hints, &result);
    if (err != 0)
    {
        errx(err, "getaddrinfo(): %s", gai_strerror(err));
    }

    int sfd = -1;
    for (struct addrinfo *p = result; p != NULL; p = p->ai_next)
    {
        sfd = socket(
            p->ai_family,
            p->ai_socktype,
            p->ai_protocol);
        if (sfd == -1)
        {
            continue;
        }
        int opt_val = 1;
        err = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));
        if (err != 0)
        {
            errx(err, "setsockopt()");
        }
        if (bind(sfd, p->ai_addr, p->ai_addrlen) == 0)
        {
            break;
        }
        close(sfd);
    }
    freeaddrinfo(result);
    if (sfd == -1)
    {
        errx(sfd, "socket(), bind()");
    }

    err = listen(sfd, 5);
    if (err != 0)
    {
        errx(-1, "listen()");
    }
    printf("Dynamic Server\n");
    printf("Listening to port 2048...\n");
    while (1)
    {
        int *accepted_sfd = (int *)malloc(sizeof(int));
        if (accepted_sfd == NULL)
        {
            errx(-1, "malloc()");
        }
        *accepted_sfd = accept(sfd, NULL, NULL);
        if (*accepted_sfd == -1)
        {
            close(sfd);
            errx(-1, "accept()");
        }
        pthread_t thread;
        err = pthread_create(&thread, NULL, worker, accepted_sfd);
        if (err != 0)
        {
            errx(err, "pthread_create()");
        }
        err = pthread_detach(thread);
        if (err != 0)
        {
            errx(err, "pthread_detach()");
        }
    }
    close(sfd);
    pthread_mutex_destroy(&lock);
    return 0;
}
