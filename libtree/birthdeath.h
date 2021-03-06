#ifndef __BIRTHDEATH_H__
#define __BIRTHDEATH_H__

#include <assert.h>
#include "hashtable.h"
#include "chooseln_cache.h"

struct square_matrix {
	double *values;
	int size;
};
void square_matrix_init(struct square_matrix* matrix, int sz);
void square_matrix_set(struct square_matrix* matrix, int x, int y, double val);
void square_matrix_resize(struct square_matrix* matrix, int new_size);
void square_matrix_multiply(struct square_matrix* matrix, double *vector, int row_start, int row_end, int col_start, int col_end, double *result);

static inline double square_matrix_get(struct square_matrix *matrix, int x, int y)
{
	assert(x < matrix->size);
	assert(y < matrix->size);
	return matrix->values[x*matrix->size+y];
}



struct BirthDeathCacheKey
{
	int branchlength;
	double lambda;
	double mu;
};

/**
* \brief A cache of values of family size transition probabilities
*
* table keys are BirthDeathCacheKeys, which hold values for branch length, lambda, and mu
* Values are a square matrix which hold transition probablities from size i to size j
*/
typedef struct
{
	/// 
    hash_table_t* table;
	int maxFamilysize;
}BirthDeathCacheArray;
typedef BirthDeathCacheArray* pBirthDeathCacheArray;

extern void birthdeath_cache_array_free(pBirthDeathCacheArray pbdc_array);
extern double birthdeath_likelihood_with_s_c(int s, int c, double branchlength, double lambda, double mu, struct chooseln_cache *cache);
extern struct square_matrix* compute_birthdeath_rates( double branchlength, double lambda, double mu, int maxFamilysize );
struct square_matrix* birthdeath_cache_get_matrix(pBirthDeathCacheArray pbdc_array, double branchlength, double lambda, double mu );
extern void thread_run(int numthreads, void* (*run)(void*), void* param, int size );
double birthdeath_rate_with_log_alpha(int s, int c, double log_alpha, double coeff, struct chooseln_cache *cache);
extern void birthdeath_cache_resize(pBirthDeathCacheArray pbdc_array, int remaxFamilysize);
pBirthDeathCacheArray birthdeath_cache_init(int size);

/**
* \brief A cache of values of chooseln
*
* Chooseln evaluates the natural logarithm of Gamma(n+1)/(Gamma(k+1)*Gamma(n-k+1))
* The cache holds values for integer values of n and k. It does not appear to be 
* threadsafe.
*/
extern int chooseln_is_init();
extern int get_chooseln_cache_size();
extern void chooseln_cache_init(int size);
extern void chooseln_cache_resize(int resize);
extern void chooseln_cache_free();
#endif
