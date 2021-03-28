/**
 * \file hashes_v2.h
 * \brief Generic hash table with Cuckoo hashing support -- hash functions.
 * \author Roman Vrana, xvrana20@stud.fit.vutbr.cz
 * \date 2013
 */
#ifndef HASHES_H
#define HASHES_H
#include "../include/cuckoo_hash_v2.h"

/*
 * Hash functions used for the table.
 * You can make your own functions simply by changing their internal code.
 */

extern unsigned int hash_1(char* key, unsigned int key_length, unsigned int t_size);

extern unsigned int hash_2(char* key, unsigned int key_length, unsigned int t_size);

extern unsigned int hash_3(char* key, unsigned int key_length, unsigned int t_size);
#endif
