#ifndef IPC_UTILS_H
#define IPC_UTILS_H

void createUserPipe(const char *username);
void notifyUser(const char *username, const char *message);
void *notificationListener(void *arg);

#endif // IPC_UTILS_H
