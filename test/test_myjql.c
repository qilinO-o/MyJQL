#include "b_tree.h"
#include "myjql.h"
#include "block.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#ifdef _WIN32
typedef long long my_off_t;
#else
typedef long my_off_t;
#endif

void init();
void insert(char *k, char *v);
void erase(char *k);
void get_value(char *k, char *v);
int contain(char *k);
int get_rand_key(char *k);
size_t get_total();

RID get_rand_rid() {
    RID rid;
    get_rid_block_addr(rid) = rand();
    get_rid_idx(rid) = (short)rand();
    return rid;
}

#define N 1024

char buf1[N + 1];
char buf2[N + 1];
char buf3[N + 1];

char random_char() {
    int op = rand() % 3;
    if (op == 0) {
        return 'a' + rand() % 26;
    } else if (op == 1) {
        return 'A' + rand() % 26;
    } else {
        return '0' + rand() % 10;
    }
}

int generate_string_key(int n) {
    int len = rand() % n;
    if(len == 0) len = 1;
    int i;
    for (i = 0; i < len; ++i) {
        buf1[i] = random_char();
    }
    buf1[len] = 0;
    return len;
}
int generate_string_value(int n) {
    int len = rand() % n;
    int i;
    for (i = 0; i < len; ++i) {
        buf2[i] = random_char();
    }
    buf2[len] = 0;
    return len;
}


int test(int num_op, int out)
{
    int flag = 0;
    int i, op, n1, n2;
    RID rid, rid1, rid2;
    myjql_init();

    init();

    for (i = 0; i < num_op; ++i) {
        if(i<num_op/2) op = 0;
        else op = 1 + rand() % 2;
        
        if (op == 0) {  /* insert */
            do {
                n1 = generate_string_key(20);
            } while (contain(buf1));
            if (out) {
                printf("insert: ");
                printf("key: %s ", buf1);
                printf("value: %s ", buf2);
                printf("\n");
            }
            n2 = generate_string_value(20);
            myjql_set(buf1, n1, buf2, n2);
            insert(buf1, buf2);
        } else if (op == 1 && get_total() != 0) {  /* erase */
            n1 = get_rand_key(buf1);
            if(n1 == -1) continue;
            if (out) {
                printf("erase: ");
                printf("%s", buf1);
                printf("\n");
            }
            myjql_del(buf1,n1);
            erase(buf1);
        } else if (op == 2 && get_total() != 0){  /* find */
            n1 = get_rand_key(buf1);
            if(n1 == -1) continue;
            if (out) {
                printf("search: ");
                printf("%s", buf1);
                printf("\n");
            }
            
            
            if (n1 != -1 && contain(buf1)) {
                n2 = myjql_get(buf1, n1, buf2, 100);
                get_value(buf1, buf3);
            } else {
                buf3[0] = 0;
                buf2[0] = 0;
            }
            if (out) {
                printf("expected: ");
                printf("%s", buf3);
                printf(", got: ");
                printf("%s", buf2);
                printf("\n");
            }

            if (strcmp(buf2,buf3) == 0) {
                if (out) {
                    printf("OK\n");
                }
            } else {
                printf("* error: \n");
                printf("expected: ");
                printf("%s", buf3);
                printf(", got: ");
                printf("%s", buf2);
                printf("\n");
                flag = 1;
                break;  /* for */
            }
        }
    }

    /* b_tree_traverse(&pool); */

    /* validate_buffer_pool(&pool); */
    myjql_close();

    return flag;
}

int main()
{
    printf("PAGE_SIZE = %d\n", PAGE_SIZE);
    printf("DEGREE = " FORMAT_SIZE_T "\n", DEGREE);
    printf("BNode size: " FORMAT_SIZE_T "\n", sizeof(BNode));
    if (DEGREE < 2) {
        printf("error: DEGREE < 2\n");
        return 1;
    }
    if (sizeof(BNode) > PAGE_SIZE) {
        printf("error: BNode size is too large\n");
        return 1;
    }
    
    /* fixed random seed */
    srand(time(NULL));
    // srand((unsigned int)time(NULL));

    // if (test(400, 1)) {
    //     return 1;
    // }
    if (test(50, 1)) {
        return 1;
    }

    /* prevent using exit(0) to pass the test */
    printf("END OF TEST\n");
    return 0;
}