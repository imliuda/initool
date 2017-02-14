#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <assert.h>

typedef enum {ADD, UPDATE, DELETE, GET} Action;
typedef enum {ITEM, SECTION, COMMENT, UNKNOWN} LineType;
typedef struct _Line Line;

typedef struct {
    char *delimiter;
} Inicfg;

struct _Line {
    char        *ptr;
    LineType    type;
    void        *data;  // item name, section name
    void        *data1; // item value
    void        *data2; // item section
    void        *data3; // delimiter
    Line        *prev;
    Line        *next;
};

Line *list_new() {
    Line *p = malloc(sizeof(Line));
    p->next = p;
    p->prev = p;
    return p;
}

Line *list_append(Line *list, Line *line) {
    list->prev->next = line;
    line->prev = list->prev;
    line->next = list;
    list->prev = line;
    return list;
}

Line *ini_load(const char *filename, const Inicfg *cfg) {
    FILE *fp = fopen(filename, "rw");
    size_t nouse;
    regmatch_t  pmatch[8];
    regex_t     preg_section;
    regex_t     preg_comment;
    regex_t     preg_item;

    char *RE_SECTION = "^[[:blank:]]*\\[(.+?)\\][[:blank:]]*";
    char *RE_COMMENT = "^[[:blank:]]*([;#]{1,1}).*";
    char RE_ITEM[256];
    sprintf(RE_ITEM, "^[[:blank:]]*([a-zA-Z0-9_-]+)[[:blank:]]*([%s])[[:blank:]]*(.*)", cfg->delimiter);

    if (regcomp(&preg_comment, RE_COMMENT, REG_EXTENDED | REG_NEWLINE) != 0) {
        perror("regcomp");
    }
    if (regcomp(&preg_section, RE_SECTION, REG_EXTENDED | REG_NEWLINE) != 0) {
        perror("regcomp");
    }
    if (regcomp(&preg_item, RE_ITEM, REG_EXTENDED | REG_NEWLINE) != 0) {
        perror("regcomp");
    }

    Line *list = list_new();
    char *cur_section = "";
    char *line = NULL;
    while (getline(&line, &nouse, fp) != -1) {
        Line *l = malloc(sizeof(Line));
        l->ptr = line;
        if (regexec(&preg_comment, line, 2, pmatch, 0) == 0) {
            l->type = COMMENT;
        } else if (regexec(&preg_section, line, 2, pmatch, 0) == 0) {
            l->type = SECTION;
            char *section = strndup(line + pmatch[1].rm_so, pmatch[1].rm_eo - pmatch[1].rm_so);
            cur_section = strdup(section);
            l->data = section;
        } else if (regexec(&preg_item, line, 4, pmatch, 0) == 0) {
            l->type = ITEM;
            char *item = strndup(line + pmatch[1].rm_so, pmatch[1].rm_eo - pmatch[1].rm_so);
            char *value = strndup(line + pmatch[3].rm_so, pmatch[3].rm_eo - pmatch[3].rm_so);
            char *delimiter = strndup(line + pmatch[2].rm_so, pmatch[2].rm_eo - pmatch[2].rm_so);
            l->data = item;
            l->data1 = value;
            l->data2 = strdup(cur_section);
            l->data3 = delimiter;
        } else {
            l->type = UNKNOWN;
        }
        list_append(list, l);
        line = NULL;
    }

    regfree(&preg_section);
    regfree(&preg_comment);
    regfree(&preg_item);
    assert (list != NULL);
    fclose(fp);
    return list;
}

void ini_save(Line *list, char *filename) {
    FILE *fp = fopen(filename, "w+");
    if (fp == NULL) {
        perror("error open file");
    }
    Line *ptr = list->next;
    while(ptr != list) {
            fputs(ptr->ptr, fp);
            //printf("%s", ptr->ptr);
            ptr = ptr->next;
    }
    fclose(fp);
}

void check_action(Action action) {
    if (action != -1) {
        fprintf(stderr, "%s", "can only assign one action\n"); 
        exit(1);
    }
}

/* initool -a config.ini section item value
 *
 */
int main(int argc, char *argv[]) {
    Action action = -1;
    char *filename = "", *section = "", *item = "", *value = "", *delimiter = "=";
    char opt;
    opterr = 0;
    while ((opt = getopt(argc, argv, "audgs:b:h")) != -1) {
        switch (opt) {
        case 'a':
            check_action(action);
            action = ADD;
            break;
        case 'u':
            check_action(action);
            action = UPDATE;
            break;
        case 'd':
            check_action(action);
            action = DELETE;
            break;
        case 'g':
            check_action(action);
            action = GET;
            break;
        case 's':
            section = strdup(optarg);
            break;
        case 'b':
            delimiter = strdup(optarg);
            break;
        case 'h':
            printf("A tool for manage ini format config file, it "
                   "can add/update/delete/get config options in global area and particular sections.\n\n"
                   "Usage: initool option filename [-s section] [-f delimiter] name [value]\n\n"
                   "Accepted option include:\n-a add option\n-d delete option\n-u update option\n-g get option\n\n"
                   "Section option:\n-s section name, optional\n"
                   "Delimiter option:\n-b default is \"=\"\n");
            exit(0);
        case '?' || ':':
            fprintf(stderr, "%s", "cmmandline options error\n");
            exit(1);
        }
    }
    if (argc > optind) {
        filename = strdup(argv[optind++]);
    }
    if (argc > optind) {
        item = strdup(argv[optind++]);
    }
    if (argc > optind) {
        value = strdup(argv[optind++]);
    }
    if (strcmp(filename, "") ==0 || strcmp(item, "") == 0) {
        fprintf(stderr, "options error, please view the usage.\n");
        exit(1);
    }
    if (access(filename, R_OK | W_OK) != 0) {
        fprintf(stderr, "file does't exits\n");
        exit(1);
    }
    //printf("action: %d, file: %s, section: %s, item: %s, value: %s\n", action, filename, section, item, value);
    Inicfg cfg;
    cfg.delimiter = strdup(delimiter);
    Line *head = ini_load(filename, &cfg);

    if (action == ADD) {
        Line *line = malloc(sizeof(Line));
        line->data = strdup(item);
        line->data1 = strdup(value);
        line->data2 = strdup(section);
        line->data3 = strlen(delimiter) > 0 ? delimiter : strdup("=");
        line->type = ITEM;
        char buf[2048];
        strcpy(buf, item);
        strcat(buf, delimiter);
        strcat(buf, value);
        strcat(buf, "\n");
        line->ptr = strdup(buf);

        Line *ptr = head->next;
        Line *after = NULL;
        while (ptr != head) {
            if (strcmp(section, "") == 0) {
                // add when section begin or end of file
                if (ptr->type == SECTION) {
                    ptr->prev->next = line;
                    line->prev = ptr->prev;
                    line->next = ptr;
                    ptr->prev = line;
                    break;
                }
                if (ptr->next == head) {
                    ptr->next->prev = line;
                    line->next = ptr->next;
                    line->prev = ptr;
                    ptr->next = line;
                    break;
                }
            } else {
                if ((ptr->type == SECTION && strcmp(ptr->data, section) == 0) ||
                    (ptr->type == ITEM && strcmp(ptr->data2, section) == 0)) {
                    after = ptr;
                }
                if (ptr->type == SECTION && strcmp(section, ptr->data) != 0 && after != NULL ||
                    ptr->next == head && after != NULL) {
                    after->next->prev = line;
                    line->next = after->next;
                    line->prev = after;
                    after->next = line;
                    break;
                }
            }
            ptr = ptr->next;
        }     
    }
    // may be have some problem
    if (action == DELETE) {
        Line *ptr = head->next;
        while (ptr != head) {
            if (ptr->type == ITEM && strcmp(section, ptr->data2) == 0 && strcmp(item, ptr->data) == 0) {
                ptr->prev->next = ptr->next;
                ptr->next->prev = ptr->prev;
                // todo free

            }
            ptr = ptr->next;
        }
    }
    if (action == UPDATE) {
        Line *ptr = head->next;
        while (ptr != head) {
            if (ptr->type == ITEM && strcmp(section, ptr->data2) == 0 && strcmp(item, ptr->data) == 0) {
                ptr->data1 = strdup(value);
                
                char buf[2048];
                strcpy(buf, item);
                strcat(buf, delimiter);
                strcat(buf, value);
                strcat(buf, "\n");
                ptr->ptr = strdup(buf);
            }
            ptr = ptr->next;
        }
    }
    if (action == GET && head != NULL) {
        Line *ptr = head->next;
        while (ptr != head) {
            if (ptr->type == ITEM && strcmp(section, ptr->data2) == 0 && strcmp(item, ptr->data) == 0) {
                printf("%s\n", ptr->data1);
            }
            ptr = ptr->next;
        }
    }
    //ini_save(head, "/tmp/inifile");
    ini_save(head, filename);
    exit(0);
}
