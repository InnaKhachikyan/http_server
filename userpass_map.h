#ifndef USERPASS_MAP_H
#define USERPASS_MAP_H

#include <stdbool.h>
#include <stddef.h>

struct User {
	char *username;
	char *password;
};

enum {
	ADD_USER_OK = 0,
	ADD_USER_EXISTS = 1,
	ADD_USER_ERR = -1
};

enum {
	DELETE_USER_OK = 0,
	DELETE_USER_NOT_FOUND = 1
};

extern struct User *entries;
extern size_t user_count;
extern size_t capacity;

void init_user_map(void);
void free_user_map(void);

int add_user(const char *user, const char *pass);
int delete_user(const char *user);

bool authenticate_user(const char *user, const char *pass);

#endif
