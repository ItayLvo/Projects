/*
Date: 26/6
Status: done after fixing CR comments
Reviewer: Tal
*/

#include <stddef.h>	/* size_t */
#include <assert.h>	/* assert */
#include <stdlib.h>	/* malloc, free */
#include <math.h>	/* pow, sqrt */

#include "hash_table.h"	/* hash table function declerations */
#include "dllist.h"	/* doubley linked list data structure and functions */


struct hash_table
{
	dll_t **buckets;
	hash_func_t hash_func;
	hash_cmp_func_t compare_func;
	size_t hash_table_size;
};


/* wrapper to client's key+data pair */
typedef struct
{
	const void *key;
	void *data;
} element_t;


/* static functions forward declerations */
static element_t *CreateHashTableElement(const void *key, void *data);
static dll_iterator_t HashTableFindElementInBucket(const hash_table_t *table, const void *key);
static dll_t *GetBucketByKey(const hash_table_t *table, const void *key);


/**** API functions ****/

hash_table_t *HashTableCreate(hash_func_t hash_func, hash_cmp_func_t cmp_func, size_t hash_table_size)
{
	size_t i = 0;
	hash_table_t *table = NULL;
	dll_t **bucket_runner = NULL;
	
	assert(hash_table_size);
	assert(hash_func);
	assert(cmp_func);
	
	table = (hash_table_t*)malloc(sizeof(hash_table_t));
	
	if (NULL == table)
	{
		return NULL;
	}
	
	table->buckets = (dll_t **)malloc(hash_table_size * sizeof(dll_t *));
	if (NULL == table->buckets)
	{
		free(table);
		return NULL;
	}
	
	
	bucket_runner = table->buckets;
	/* create a DLL for each "bucket" */
	for (i = 0; i < hash_table_size; ++i, ++bucket_runner)
	{
		*bucket_runner = DLListCreate();

		/* check if bucket successfully created. if not, free the table and all buckets allocated so far */
		if (NULL == *bucket_runner)
		{
			size_t j = 0;
			bucket_runner = table->buckets;
			for (j = 0; j < i; ++j, ++bucket_runner)
			{
				DLListDestroy(*bucket_runner);
			}
			free(table);
			return NULL;
		}
	}
	
	table->hash_func = hash_func;
	table->compare_func = cmp_func;
	table->hash_table_size = hash_table_size;
	
	return table;
}




void HashTableDestroy(hash_table_t *table)
{
	size_t i = 0;
	dll_t *current_bucket = NULL;
	dll_iterator_t list_runner = {0};
	element_t *current_element = NULL;
	dll_t **bucket_runner = NULL;
	
	assert(table);
	
	bucket_runner = table->buckets;
	for (i = 0; i < table->hash_table_size; ++i, ++bucket_runner)
	{
		current_bucket = *bucket_runner;
		list_runner = DLListBegin(current_bucket);
		
		while (!DLListIsEqualIter(list_runner, DLListEnd(current_bucket)))
		{
			current_element = (element_t *)DLListGetData(list_runner);
			free(current_element);
			list_runner = DLListNext(list_runner);
		}
		
		DLListDestroy(current_bucket);
	}
	
	free(table->buckets);
	free(table);
}



int HashTableInsert(hash_table_t *table, const void *key, void *data)
{
	dll_t *current_bucket = NULL;
	dll_iterator_t insert_status = {0};
	element_t *new_element = NULL;
	
	assert(table);
	
	current_bucket = GetBucketByKey(table, key);

	/* create element struct to wrap key-data pair */
	new_element = CreateHashTableElement(key, data);
	if (NULL == new_element)
	{
		return 1;
	}
	
	/* insert the element (key-data pair) to the relevant bucket */
	insert_status = DLListInsert(current_bucket, DLListBegin(current_bucket), new_element);
	
	/* check DLL insert validity */
	if (DLListIsEqualIter(insert_status, DLListEnd(current_bucket)))
	{
		free(new_element);
		return 1;
	}
	
	return 0;
}



void HashTableRemove(hash_table_t *table, const void *key)
{
	element_t *element_to_remove = NULL;
	dll_iterator_t iterator_to_remove = {0};
	dll_t *current_bucket = NULL;
	
	assert(table);
	
	current_bucket = GetBucketByKey(table, key);
	iterator_to_remove = HashTableFindElementInBucket(table, key);
	
	if (DLListIsEqualIter(iterator_to_remove, DLListEnd(current_bucket)))
	{
		element_to_remove = DLListGetData(iterator_to_remove);
		free(element_to_remove);
		DLListRemove(iterator_to_remove);
	}
}



void *HashTableFind(const hash_table_t *table, const void *key)
{
	dll_iterator_t iterator_to_find = HashTableFindElementInBucket(table, key);
	dll_t *current_bucket = GetBucketByKey(table, key);
	element_t *element_to_find = NULL;
	void *data_to_find = NULL;
	dll_iterator_t insert_status = {0};
	
	if (NULL == iterator_to_find || DLListIsEqualIter(iterator_to_find, DLListEnd(current_bucket)))
	{
		return NULL;
	}
	
	element_to_find = DLListGetData(iterator_to_find);
	data_to_find = element_to_find->data;
	
	/* caching the found element: re-inserting it to the start of the bucket */
	DLListRemove(iterator_to_find);
	insert_status = DLListInsert(current_bucket, DLListBegin(current_bucket), element_to_find);
	if (DLListIsEqualIter(insert_status, DLListEnd(current_bucket)))
	{
		return NULL;
	}
	
	return data_to_find;
}



size_t HashTableSize(const hash_table_t *table)
{
	size_t i = 0;
	size_t size_sum = 0;
	dll_t *current_bucket = NULL;
	dll_t **bucket_runner = NULL;
	
	assert(table);
	
	bucket_runner = table->buckets;
	for (i = 0; i < table->hash_table_size; ++i, ++bucket_runner)
	{
		current_bucket = *bucket_runner;
		size_sum += DLListCount(current_bucket);
	}
	
	return size_sum;
}


int HashTableIsEmpty(const hash_table_t *table)
{
	size_t i = 0;
	dll_t *current_bucket = NULL;
	dll_t **bucket_runner = NULL;
	
	assert(table);
	
	bucket_runner = table->buckets;
	for (i = 0; i < table->hash_table_size; ++i, ++bucket_runner)
	{
		current_bucket = *bucket_runner;
		if (!DLListIsEmpty(current_bucket))
		{
			return 0;
		}
	}
	
	return 1;
}



int HashTableForEach(hash_table_t *table, hash_action_func_t action_func, void *params)
{
	size_t i = 0;
	int action_func_status = 0;
	dll_t *current_bucket = NULL;
	dll_t **bucket_runner = NULL;
	dll_iterator_t list_runner = {0};
	element_t *element = NULL;
	
	assert(table);
	assert(action_func);
	
	bucket_runner = table->buckets;
	
	for (i = 0; i < table->hash_table_size; ++i, ++bucket_runner)
	{
		current_bucket = *bucket_runner;
		list_runner = DLListBegin(current_bucket);
		
		while (!DLListIsEqualIter(list_runner, DLListEnd(current_bucket)))
		{
			element = ((element_t *)DLListGetData(list_runner));
			action_func_status = (action_func)(element->data, params);
			
			if (0 != action_func_status)
			{
				return action_func_status;
			}
			
			list_runner = DLListNext(list_runner);
		}
	}
	
	return action_func_status;
}



double HashTableLoad(const hash_table_t *table)
{
	double num_of_elements_in_table = 0;
	double num_of_buckets = 0;
	
	assert(table);
	
	num_of_elements_in_table = (double)HashTableSize(table);
	num_of_buckets = (double)table->hash_table_size;
	
	return num_of_elements_in_table / num_of_buckets;
}



double HashTableStandardDeviation(const hash_table_t *table)
{
	size_t i = 0;
	double variance = 0;
	double standard_deviation = 0;
	dll_t *current_bucket = NULL;
	double avg_element_per_bucket = HashTableLoad(table);
	dll_t **bucket_runner = NULL;

	bucket_runner = table->buckets;
	for (i = 0; i < table->hash_table_size; ++i, ++bucket_runner)
	{
		current_bucket = *bucket_runner;
		variance += pow((DLListCount(current_bucket) - avg_element_per_bucket), 2);
	}
	
	variance /= table->hash_table_size;
	standard_deviation = sqrt(variance);
	
	return standard_deviation;
}




/**** static helper functions ****/

static dll_iterator_t HashTableFindElementInBucket(const hash_table_t *table, const void *key)
{
	dll_t *current_bucket = GetBucketByKey(table, key);
	
	hash_cmp_func_t client_compare_func = table->compare_func;
	int match_result = 0;
	dll_iterator_t list_runner = DLListBegin(current_bucket);
	element_t *current_element = NULL;
	
	while (!DLListIsEqualIter(list_runner, DLListEnd(current_bucket)) && match_result == 0)
	{
		current_element = (element_t *)DLListGetData(list_runner);
		match_result = client_compare_func(current_element->key, key);
		list_runner = DLListNext(list_runner);
	}
	
	return ((match_result == 0) ? NULL : DLListPrev(list_runner));
}



static element_t *CreateHashTableElement(const void *key, void *data)
{
	element_t *element = (element_t *)malloc(sizeof(element_t));
	if (NULL == element)
	{
		return NULL;
	}
	
	element->key = key;
	element->data = data;
	
	return element;
}



static dll_t *GetBucketByKey(const hash_table_t *table, const void *key)
{
	size_t hash_result = (table->hash_func)(key);
	return (table->buckets)[hash_result % table->hash_table_size];
}


