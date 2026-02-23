#include "ipc_utils.h"
#include "../ui/tui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <ctype.h>

static void _get_fifo_path(const char *user, char *path, size_t size) {
    char lower[64];
    size_t i;
    for (i = 0; i < sizeof(lower)-1 && user[i]; i++)
        lower[i] = (char)tolower((unsigned char)user[i]);
    lower[i] = '\0';
    snprintf(path, size, "/tmp/atm_fifo_%s", lower);
}

void createUserPipe(const char *username) {
    char fifoPath[256];
    _get_fifo_path(username, fifoPath, sizeof(fifoPath));

    if (mkfifo(fifoPath, 0666) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo");
        }
    }
}

void notifyUser(const char *username, const char *message) {
    char fifoPath[256];
    _get_fifo_path(username, fifoPath, sizeof(fifoPath));

    int fd = open(fifoPath, O_WRONLY | O_NONBLOCK);
    if (fd == -1) {
        FILE *f = fopen("/tmp/atm_debug.log", "a");
        if (f) {
            fprintf(f, "SEND FAILED to %s: %s\n", username, strerror(errno));
            fclose(f);
        }
        return;
    }

    write(fd, message, strlen(message));
    close(fd);
}

void *notificationListener(void *arg) {
    const char *username = (const char *)arg;
    char fifoPath[256];
    _get_fifo_path(username, fifoPath, sizeof(fifoPath));

    createUserPipe(username);

    int fd = open(fifoPath, O_RDONLY | O_NONBLOCK);

    if (fd == -1) {
        if (errno == ENXIO) {
            fprintf(stderr, "User %s is not listening to notifications\n", username);
        }
        return NULL;
    }

    char buffer[256];
    ssize_t n;

    while (1) {
        n = read(fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            FILE *f = fopen("/tmp/atm_debug.log", "a");
            if (f) {
                fprintf(f, "RECEIVED NOTIF: %s\n", buffer);
                fclose(f);
            }
            tui_set_notification(buffer);
        }
        sleep(1);
    }

    close(fd);
    return NULL;
}
