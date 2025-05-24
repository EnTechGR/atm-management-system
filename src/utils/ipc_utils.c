#include "ipc_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>

void createUserPipe(const char *username) {
    char fifoPath[256];
    snprintf(fifoPath, sizeof(fifoPath), "/tmp/atm_fifo_%s", username);

    if (mkfifo(fifoPath, 0666) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo");
        }
    }
}

void notifyUser(const char *username, const char *message) {
    char fifoPath[256];
    snprintf(fifoPath, sizeof(fifoPath), "/tmp/atm_fifo_%s", username);

    int fd = open(fifoPath, O_WRONLY | O_NONBLOCK);
    if (fd == -1) {
        // User may be offline or pipe not opened
        return;
    }

    write(fd, message, strlen(message));
    close(fd);
}

void *notificationListener(void *arg) {
    const char *username = (const char *)arg;
    char fifoPath[256];
    snprintf(fifoPath, sizeof(fifoPath), "/tmp/atm_fifo_%s", username);

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
            printf("\n*** Notification: %s\n", buffer);
            printf("Please choose an option (1-8): ");  // or your prompt here
            fflush(stdout);
        }
        sleep(1);
    }

    close(fd);
    return NULL;
}
