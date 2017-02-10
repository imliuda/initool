#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <assert.h>

#define RE_SECTION  "[:blank:]*\\[(.+?)\\][:black:]*"
#define RE_COMMENT  "[:blank:]*([;#]{1,1}).*"
#define RE_ITEM     "[:blank:]*([a-zA-Z0-9_-]+)[:blank:]*=[:blank:]*(.*)"

typedef enum {ADD, UPDATE, DELETE, GET} Action;
typedef enum {ITEM, SECTION, COMMENT, UNKNOWN} LineType;
typedef struct _Line Line;

struct _Line {
    char        *ptr;
    LineType    type;
    void        *data;  // item name, section name
    void        *data1; // item value
    void        *data2; // item section
    Line        *prev;
    Line        *next;
};

Line *list_new() {
    Line *p = NULL;
    return p;
}

Line *list_append(Line *list, Line *line) {
    if (list == NULL) {
        list = line;
        list->prev = list;
        list->next = list;
    } else {
        list->prev->next = line;
        line->prev = list->prev;
        line->next = list;
        list->prev = line;
    }
}

Line *ini_load(const char *filename) {
    FILE *fp = fopen(filename, "rw");
    size_t nouse;
    regmatch_t  pmatch[8];
    regex_t     preg_section;
    regex_t     preg_comment;
    regex_t     preg_item;
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
        if (regexec(&preg_comment, line, 2, pmatch, REG_NOTBOL | REG_NOTEOL) == 0) {
            l->type = COMMENT;
        } else if (regexec(&preg_section, line, 2, pmatch, REG_NOTBOL | REG_NOTEOL) == 0) {
            l->type = SECTION;
            char *section = strndup(line + pmatch[1].rm_so, pmatch[1].rm_eo - pmatch[1].rm_so);
            cur_section = strdup(section);
            l->data = section;
        } else if (regexec(&preg_item, line, 3, pmatch, REG_NOTBOL | REG_NOTEOL) == 0) {
            l->type = ITEM;
            char *item = strndup(line + pmatch[1].rm_so, pmatch[1].rm_eo - pmatch[1].rm_so);
            char *value = strndup(line + pmatch[2].rm_so, pmatch[2].rm_eo - pmatch[2].rm_so);
            l->data = item;
            l->data1 = value;
            l->data2 = strdup(cur_section);
        } else {
            l->type = UNKNOWN;
        }
        list = list_append(list, l);
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
    Line *ptr = list;
    if (list != NULL) {
        do {
            fputs(ptr->ptr, fp);
            //printf("%s", ptr->ptr);
            ptr = ptr->next;
        } while (list != ptr);
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
    char *filename = "", *section = "", *item = "", *value = "";
    char opt;
    opterr = 0;
    while ((opt = getopt(argc, argv, "audgs:h")) != -1) {
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
        case 'h':
            printf("A tool for manage ini format config file, ii"
                   "t can add/update/delete/get config options in global area and particular sections.\n\n"
                   "Usage: initool option filename [-s] name [value]\n\n"
                   "Accepted option include:\n-a add option\n-d delete option\n-u update option\n-g get option\n\n"
                   "Section option:\n-s section name, optional\n");
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
    //printf("action: %d, file: %s, section: %s, item: %s, value: %s\n", action, filename, section, item, value);
    Line *head = ini_load(filename);

    if (action == ADD && head != NULL) {
        Line *line = malloc(sizeof(Line));
        line->data = strdup(item);
        line->data1 = strdup(value);
        line->data2 = strdup(section);
        line->type = ITEM;
        char buf[1024];
        strcpy(buf, item);
        strcat(buf, "=");
        strcat(buf, value);
        strcat(buf, "\n");
        line->ptr = strdup(buf);

        if (strcmp(section, "") == 0) {
            // replace head
            head->prev->next = line;
            line->prev = head->prev;
            line->next = head;
            head->prev = line;
            head = line;
        } else {
            Line *ptr = head;
            do {
                if (ptr->type == SECTION && strcmp(section, ptr->data) == 0) {
                    ptr->next->prev = line;
                    line->prev = ptr;
                    line->next = ptr->next;
                    ptr->next = line;
                    break;
                }
                ptr = ptr->next;
            } while (head != ptr);           
        }
    }
    // may be have some problem
    if (action == DELETE && head != NULL) {
        Line *ptr = head;
        do {
            if (ptr->type == ITEM && strcmp(section, ptr->data2) == 0 && strcmp(item, ptr->data) == 0) {
                ptr->prev->next = ptr->next;
                ptr->next->prev = ptr->prev;
                // todo free
                if (head == ptr) {
                    head = ptr->next;
                }
            }
            ptr = ptr->next;
        } while (head != ptr);
    }
    if (action == UPDATE && head != NULL) {
        Line *ptr = head;
        do {
            if (ptr->type == ITEM && strcmp(section, ptr->data2) == 0 && strcmp(item, ptr->data) == 0) {
                ptr->data1 = strdup(value);
                
                char buf[1024];
                strcpy(buf, item);
                strcat(buf, "=");
                strcat(buf, value);
                strcat(buf, "\n");
                ptr->ptr = strdup(buf);
            }
            ptr = ptr->next;
        } while (head != ptr);
    }
    if (action == GET && head != NULL) {
        Line *ptr = head;
        do {
            if (ptr->type == ITEM && strcmp(section, ptr->data2) == 0 && strcmp(item, ptr->data) == 0) {
                printf("%s ", ptr->data1);
            }
            ptr = ptr->next;
        } while (head != ptr);
        printf("\n");
    }
    //ini_save(head, "/tmp/inifile");
    ini_save(head, filename);
    exit(0);
}
