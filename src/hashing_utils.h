// hashing_utils.h
#ifndef HASHING_UTILS_H
#define HASHING_UTILS_H

#include <stddef.h> // For size_t

void generateSalt(unsigned char *salt, size_t length);
void bin2hex(unsigned char *bin, size_t bin_len, char *hex);
void hashPassword(const char *password, const unsigned char *salt, size_t salt_len, unsigned char *hash);
int verifyPassword(const char *password, const char *stored_password);

#endif // HASHING_UTILS_H