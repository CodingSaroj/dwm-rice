#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <unistd.h>

#define SUTL_IMPLEMENTATION
#include "ShroonUtils/include/Shroon/Utils/Vector.h"

/* A search entry */

typedef struct {
    char * name;
    char * file;
    char * command;
} entry;

entry entries[4096] = {0};

/* Concatenate s1 and s2 into a new allocated string while preserving both s1 and s2 */

char * pstrcat(const char * s1, const char * s2) {
    int l1 = strlen(s1), l2 = strlen(s2);

    char * res = malloc(l1 + l2 + 1);

    memset(res, 0, l1 + l2 + 1);
    memcpy(res, s1, l1);
    memcpy(res + l1, s2, l2);

    return res;
}

/* Returns 1 if s contains filter case-insensitively, 0 otherwise */

int filter_string(const char * s, const char * filter) {
    char * s_low = strdup(s);
    char * filter_low = strdup(filter);

    int i = 0;

    while (s_low[i] != 0) {
        s_low[i] = tolower(s_low[i]);
        i++;
    }

    i = 0;

    while (filter_low[i] != 0) {
        filter_low[i] = tolower(filter_low[i]);
        i++;
    }

    if (strlen(s) > strlen(filter)) {
        for (int i = 0; i < strlen(s) - strlen(filter); i++) {
            if (strncmp(s_low + i, filter_low, strlen(filter)) == 0) {
                free(s_low);
                free(filter_low);
                return 1;
            }
        }
    } else if (strlen(s) == strlen(filter) && strcmp(s_low, filter_low) == 0) {
        free(s_low);
        free(filter_low);
        return 1;
    }

    free(s_low);
    free(filter_low);
    return 0;
}

int main(int argc, char ** argv) {
    char ** search_targets;
    FILE * f;
    const char * home;
    char * path;
    int entry_end;
    int id;
    int printed;
    int * ids;
    char filter[256] = {0};

    /* Greeting */

    printf("\033[1;32mBisect 1.0.0\033[0m\n\n");

    search_targets = SUTLVectorNew(char *);

    /* Try to get list of search targets from the user paths file */

    home = getenv("HOME");

    path = pstrcat(home, "/.bisect");

    f = fopen(path, "r");

    free(path);

    if (f) {
        char * line = NULL;
        size_t size = 0;
        ssize_t read = 0;

        while ((read = getline(&line, &size, f)) != -1) {
            line[read - 1] = 0; /* Exclude the newline character */

            SUTLVectorPush(search_targets, line);

            line = NULL;
        }

        if (line) {
            free(line);
        }

        fclose(f);
    }

    if (SUTLVectorSize(search_targets) == 0) {
        /* Return if no search targets exist */

        printf("\033[1;31mError\033[0m: No search targets found. Make sure that $HOME/.bisect exists and is not empty. Press enter key to exit.\n\n");

        getc(stdin);

        SUTLVectorFree(search_targets);

        return 0;
    }

    /* Build the entries array by recursively searching for .desktop files in search targets */

    entry_end = 0;

    for (int i = 0; i < SUTLVectorSize(search_targets); i++) {
        DIR * d;
        struct dirent * dir;

        d = opendir(search_targets[i]);

        if (d) {
            while ((dir = readdir(d)) != NULL) {
                if (dir->d_type == DT_REG) {
                    if (strlen(dir->d_name) > 8 && strncmp(dir->d_name + strlen(dir->d_name) - 8, ".desktop", 8) == 0) {

                        /* Add an entry if 'dir' is a file and its name ends with .desktop */

                        char * tmp1, * tmp2, * tmp3;

                        tmp1 = pstrcat(search_targets[i], "/");
                        tmp2 = pstrcat(tmp1, dir->d_name);

                        entries[entry_end].name = strdup(dir->d_name);
                        entries[entry_end].file = tmp2;

                        free(tmp1);

                        tmp1 = pstrcat("cd \"", search_targets[i]);
                        tmp2 = pstrcat("\"; nohup dex --term=st \"", dir->d_name);
                        tmp3 = pstrcat(tmp2, "\"> $HOME/.bisectlog");

                        entries[entry_end].command = pstrcat(tmp1, tmp3);

                        free(tmp1);
                        free(tmp2);
                        free(tmp3);

                        entry_end++;
                    }
                } else if (dir->d_type == DT_DIR) {

                    /* Add a new search target if 'dir' is a directory */

                    if ((strlen(dir->d_name) == 1 && dir->d_name[0] == '.')
                        || (strlen(dir->d_name) == 2 && strncmp(dir->d_name, "..", 2) == 0)) {
                        continue;
                    }

                    char * tmp1 = pstrcat(search_targets[i], "/");
                    char * tmp2 = pstrcat(tmp1, dir->d_name);

                    SUTLVectorPush(search_targets, tmp2);

                    free(tmp1);
                }
            }

            closedir(d);
        }
    }

    /* Ask for query string only if no arguments are passed */

    if (argc > 1) {
        strcpy(filter, argv[1]);
    } else {
        printf("\033[1mEnter search query: \033[0m");
        scanf("%s", filter);
    }

    /* List the entries that pass the filter */

    printed = 0;

    ids = SUTLVectorNew(int);

    for (int i = 0; i < entry_end; i++) {
        if (filter_string(entries[i].name, filter)) {
            SUTLVectorPush(ids, i);
            printf("\033[1m%d.\033[0m %s\n", ++printed, entries[i].name);
        }
    }

    /* Ask for the index of the entry to open */

    if (printed > 0) {
        id = 0;

        printf("\n\033[1mEnter index: \033[0m");
        scanf("%d", &id);

        /* Open the entry at index if index is valid, else do nothing */

        if (id > 0 && id <= printed) {
            printf("\n\033[1mLaunching %s...\n", entries[ids[id - 1]].name);

            system(entries[ids[id - 1]].command);
        } else {
            printf("\n\033[1;31mInvalid id, exiting.\033[0m\n");
            sleep(1);
        }
    }

    /* Freeing allocated memory */

    for (int i = 0; i < entry_end; i++) {
        free(entries[i].name);
        free(entries[i].file);
        free(entries[i].command);
    }

    SUTLVectorEach(char *, search_targets, st, free(*st););

    SUTLVectorFree(ids);
    SUTLVectorFree(search_targets);

    return 0;
}
