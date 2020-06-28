//
// Created by cacao on 2020/6/4.
//

#include "base.h"
#include "poptrie.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#define CHK_SIZE 700000
//#define BASE

#ifdef BASE
int add_from_txt(int num, node_t *root) {
#else
int add_from_txt(int num, internal_t *root) {
#endif
    char line[256];
    int count = 0;
    FILE *fp = fopen("../forwarding-table.txt", "rb");
    if (fp == NULL) {
        printf("Open fail errno = %d. reason = %s \n", errno, strerror(errno));
        char buf[1024];
        printf("Working path : %s\n", getcwd(buf, 1024));
    }
    while (!feof(fp) && !ferror(fp)) {
        strcpy(line, "\n");
        fgets(line, sizeof(line), fp);
        u32 part1, part2, part3, part4, mask, iface;
        sscanf(line, "%u.%u.%u.%u %u %u", &part1, &part2, &part3, &part4, &mask, &iface);
        u32 dest = (part1 << 24) | (part2 << 16) | (part3 << 8) | part4;
        // printf("ip : %08x ; mask : %u ; iface : %u\n", dest, mask, iface);
#ifdef BASE
        put_base(root, iface, mask, dest);
#else
        put_poptrie(root, dest, mask, iface);
#endif
        if (++count >= num) break;
    }
    fclose(fp);
    return (count - 1);
}


#ifdef BASE
int chk_to_txt(int num, node_t *root) {
#else
int chk_to_txt(int num, internal_t *root) {
#endif
    char line[256];
    int count = 0;
    FILE *fp = fopen("../forwarding-table.txt", "rb");
    if (fp == NULL) {
        printf("Open fail errno = %d. reason = %s \n", errno, strerror(errno));
        char buf[1024];
        printf("Working path : %s\n", getcwd(buf, 1024));
    }
    while (!feof(fp) && !ferror(fp)) {
        strcpy(line, "\n");
        fgets(line, sizeof(line), fp);
        u32 part1, part2, part3, part4, mask, iface;
        sscanf(line, "%u.%u.%u.%u %u %u", &part1, &part2, &part3, &part4, &mask, &iface);
        u32 dest = (part1 << 24) | (part2 << 16) | (part3 << 8) | part4;
        //printf("ip : %08x ; mask : %u ; iface : %u\n", dest, mask, iface);
#ifdef BASE
        node_t *temp = get_base(root, dest);
#else
        leaf_t *temp = get_poptrie(root, dest);
#endif
        if (++count >= num) break;
        /*if (temp->iface != iface) {
            printf("%d\n", count);
            //break;
        }*/
    }
    fclose(fp);
    return count;
}

int main() {
#ifdef BASE
    node_t *root = init_root_base();
#else
    internal_t *root = init_root_poptrie();
#endif
    add_from_txt(CHK_SIZE, root);
#ifdef BASE
    printf("Space cost: %u Byte.\n", calc_space_base());
#else
    printf("Space cost: %u Byte\n", calc_space_poptrie());
#endif
    //return 0;
    struct  timeval begin, end;
    gettimeofday(&begin, NULL);
    printf("Items: %d\n", chk_to_txt(CHK_SIZE, root));
    gettimeofday(&end, NULL);
    printf("Time cost: %ld us.\n", 1000000 * (end.tv_sec - begin.tv_sec) + end.tv_usec - begin.tv_usec);
    return 0;
}
