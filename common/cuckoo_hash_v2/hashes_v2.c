/**
 * \file hashes.h
 * \brief Generic hash table with Cuckoo hashing support -- hash functions.
 * \author Roman Vrana, xvrana20@stud.fit.vutbr.cz
 * \date 2013
 */
#include "hashes_v2.h"

/*
 * Hash functions used for the table.
 * You can make your own functions simply by changing their internal code.
 */

unsigned int hash_1(char* key, unsigned int key_length, unsigned int t_size)
{
    unsigned int a = 63689;
    unsigned int b = 378551;
    unsigned int hash = 0;

    for (unsigned int i = 0; i < key_length; key++, i++) {
        hash = hash * a  + (*key);
        a = a * b;
    }
    return hash % t_size;
}

unsigned int hash_2(char* key, unsigned int key_length, unsigned int t_size)
{
    unsigned int hash = 1315423911;
    unsigned int i    = 0;

    for (i = 0; i < key_length; key++, i++) {
        hash ^= ((hash << 5) + (*key) + (hash >> 2));
    }

    return hash % t_size;
}

unsigned int hash_3(char* key, unsigned int key_length, unsigned int t_size)
{
    unsigned int hash = 0;
    unsigned int x    = 0;
    unsigned int i    = 0;

    for (i = 0; i < key_length; key++, i++) {
        hash = (hash << 4) + (*key);
        if((x = hash & 0xF0000000L) != 0) {
            hash ^= (x >> 24);
        }
        hash &= ~x;
    }

    return hash % t_size;
}
