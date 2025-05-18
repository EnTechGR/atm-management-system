// hashing_utils.c
#include "hashing_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>

// Function to generate a random salt
void generateSalt(unsigned char *salt, size_t length) {
    if (RAND_bytes(salt, length) != 1) {
        // If RAND_bytes fails, use a fallback method
        srand(time(NULL));
        for (size_t i = 0; i < length; i++) {
            salt[i] = rand() & 0xff;
        }
    }
}

// Function to convert binary data to hexadecimal string
void bin2hex(unsigned char *bin, size_t bin_len, char *hex) {
    static const char hexchars[] = "0123456789abcdef";
    for (size_t i = 0; i < bin_len; i++) {
        hex[i*2] = hexchars[(bin[i] >> 4) & 0xF];
        hex[i*2+1] = hexchars[bin[i] & 0xF];
    }
    hex[bin_len*2] = '\0';
}

// Function to hash a password with a salt using SHA-256
void hashPassword(const char *password, const unsigned char *salt, size_t salt_len, unsigned char *hash) {
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        perror("EVP_MD_CTX_new");
        exit(1);
    }

    if (1 != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) ||
        1 != EVP_DigestUpdate(mdctx, password, strlen(password)) ||
        1 != EVP_DigestUpdate(mdctx, salt, salt_len) ||
        1 != EVP_DigestFinal_ex(mdctx, hash, NULL)) {
        perror("EVP_Digest functions");
        EVP_MD_CTX_free(mdctx);
        exit(1);
    }

    EVP_MD_CTX_free(mdctx);
}

// Function to verify a password against a stored hash
int verifyPassword(const char *password, const char *stored_password) {
    char salt_hex[33];
    unsigned char salt[16];
    unsigned char hash[SHA256_DIGEST_LENGTH];
    char computed_hash_hex[65];
    char *hash_part;

    // Extract the salt from the stored password
    // Format of stored_password is: salt_hex$hash_hex
    strncpy(salt_hex, stored_password, 32);
    salt_hex[32] = '\0';

    // Find the hash part (after the $)
    hash_part = strchr(stored_password, '$');
    if (hash_part == NULL) {
        // Invalid format, possibly still using old plaintext format
        return strcmp(password, stored_password) == 0;
    }
    hash_part++; // Skip the $ character

    // Convert salt from hex to binary
    for (int i = 0; i < 16; i++) {
        sscanf(&salt_hex[i*2], "%2hhx", &salt[i]);
    }

    // Hash the provided password with the extracted salt
    hashPassword(password, salt, sizeof(salt), hash);

    // Convert the computed hash to hex
    bin2hex(hash, sizeof(hash), computed_hash_hex);

    // Compare the computed hash with the stored hash
    return strcmp(computed_hash_hex, hash_part) == 0;
}