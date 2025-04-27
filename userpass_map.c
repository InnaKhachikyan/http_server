#include "userpass_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

struct User *entries = NULL;
size_t user_count = 0;
size_t capacity = 0;

static int ensure_capacity(void) {
	if(user_count < capacity) {
		return 0;
	}
	size_t new_capacity = (capacity == 0) ? 10 : capacity * 2;
	struct User *temp = realloc(entries, sizeof(struct User)*new_capacity);
	if(!temp) {
		return -1;
	}
	entries = temp;
	capacity = new_capacity;
	return 0;
}

void init_user_map(void) {
	entries = NULL;
	user_count = 0;
	capacity = 0;
}

void free_user_map(void) {
	for(int i = 0; i < user_count; i++) {
		free(entries[i].username);
		free(entries[i].password);
	}
	free(entries);
	entries = NULL;
	user_count = 0;
	capacity = 0;
}

int add_user(const char *user, const char *pass) {
	for(int i = 0; i < user_count; i++) {
		if(strcmp(entries[i].username, user) == 0) {
			return ADD_USER_EXISTS;
		}
	}

	if(ensure_capacity() <0) {
		return ADD_USER_ERR;
	}

	entries[user_count].username = strdup(user);
	entries[user_count].password = strdup(pass);

	if(!entries[user_count].username || !entries[user_count].password) {
		free(entries[user_count].username);
		free(entries[user_count].password);
		return ADD_USER_ERR;
	}

	user_count++;
	return ADD_USER_OK;
}

int delete_user(const char *user) {
	for(int i =0; i < user_count; i++) {
		if(strcmp(entries[i].username, user) == 0) {
			free(entries[i].username);
			free(entries[i].password);
			memmove(&entries[i], &entries[i+1], (user_count - i - 1)*sizeof(struct User));
			user_count--;
			return DELETE_USER_OK;
		}
	}
	return DELETE_USER_NOT_FOUND;
}

bool authenticate_user(const char *user, const char *pass) {
	for(int i = 0; i < user_count; i++) {
		if(strcmp(entries[i].username, user) == 0 && strcmp(entries[i].password, pass) == 0) {
			return true;
		}
	}
	return false;
}

/*
int main(void) {
    init_user_map();

    add_user("Alice",   "0123456");
    add_user("Bob",     "987654");
    add_user("Charlie", "pass123");
    add_user("David",   "qwerty");
    add_user("Eve",     "evepass");
    add_user("Frank",   "fr4nk!");
    add_user("Grace",   "gracepwd");
    add_user("Heidi",   "heidipwd");
    add_user("Ivan",    "ivan123");
    add_user("Judy",    "judypass");

    printf("Total users added: %zu\n", user_count);
    for (size_t i = 0; i < user_count; i++) {
        printf("User %zu: %s / %s\n", i+1, entries[i].username, entries[i].password);
    }

    free_user_map();
    return 0;
}
*/
