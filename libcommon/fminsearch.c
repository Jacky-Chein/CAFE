#include<mathfunc.h>
#include<memalloc.h>

pFMinSearch fminsearch_new()
{
	pFMinSearch pfm = (pFMinSearch)memory_new(1,sizeof(FMinSearch));	
	pfm->rho = 1;				// reflection
	pfm->chi = 2;				// expansion
	pfm->psi = 0.5;				// contraction
	pfm->sigma = 0.5;			// shrink
	pfm->tolx = 1e-6;
	pfm->tolf = 1e-6;
	pfm->delta = 0.05;
	pfm->zero_delta = 0.00025;
	pfm->maxiters = 10000;
	pfm->args = NULL;
	return pfm;
}

pFMinSearch fminsearch_new_with_eq(math_func eq, int Xsize, void* args)
{
	pFMinSearch pfm = fminsearch_new();
	fminsearch_set_equation(pfm,eq,Xsize,args);
	return pfm;
}

void fminsearch_clear_memory(pFMinSearch pfm)
{
	memory_free_2dim((void**)pfm->v, pfm->N1, pfm->N, NULL);
	memory_free_2dim((void**)pfm->vsort, pfm->N1, pfm->N, NULL );
	memory_free(pfm->fv);
	pfm->fv = NULL;
	memory_free(pfm->x_mean);
	pfm->x_mean = NULL;
	memory_free(pfm->x_r);
	pfm->x_r = NULL;
	memory_free(pfm->x_tmp);
	pfm->x_tmp = NULL;
	memory_free(pfm->idx);
	pfm->idx = NULL;
}

void fminsearch_free(pFMinSearch pfm)
{
	fminsearch_clear_memory(pfm);	
	memory_free(pfm);
	pfm = NULL;
}

void fminsearch_set_equation(pFMinSearch pfm, math_func eq, int Xsize, void* args)
{
	if ( pfm->N != Xsize )
	{
		if ( pfm->eq ) fminsearch_clear_memory(pfm);
		pfm->v = (double**)memory_new_2dim(Xsize+1, Xsize, sizeof(double));
		pfm->vsort = (double**)memory_new_2dim(Xsize+1, Xsize, sizeof(double));
		pfm->fv = (double*)memory_new(Xsize+1, sizeof(double));
		pfm->x_mean = (double*)memory_new(Xsize, sizeof(double));
		pfm->x_r = (double*)memory_new(Xsize, sizeof(double));
		pfm->x_tmp = (double*)memory_new(Xsize, sizeof(double));
		pfm->idx = (int*)memory_new(Xsize+1,sizeof(int));
	}
	pfm->eq = eq;
	pfm->N = Xsize;
	pfm->N1 = Xsize + 1;
	pfm->args = args;
}


void __qsort_double_with_index(double* list, int* idx, int left, int right)
{
	double pivot = list[left];
	int pivot_idx = idx[left];
	int from = left;
	int to = right;

	while( from < to )
	{
		while( pivot <= list[to] && from < to ) to--;
		if ( from != to )
		{
			list[from] = list[to];
			idx[from] = idx[to];
			from++;
		}
		while( pivot >= list[from] && from < to ) from++;
		if ( from != to )
		{
			list[to] = list[from];
			idx[to] = idx[from];
			to--;
		}
	}
	list[from] = pivot;
	idx[from] = pivot_idx;
	if ( left < from ) __qsort_double_with_index(list,idx,left, from-1);
	if ( right > from ) __qsort_double_with_index(list,idx,from+1,right);
}

void __fminsearch_sort(pFMinSearch pfm)
{
	int i, j, k;
	for ( i = 0 ; i < pfm->N1 ; i++ ) pfm->idx[i] = i;
	__qsort_double_with_index(pfm->fv, pfm->idx, 0,pfm->N);
	for ( i = 0 ; i < pfm->N1 ; i++ )
	{
		k = pfm->idx[i];
		for( j = 0 ; j < pfm->N ; j++ )
		{
			pfm->vsort[i][j] = pfm->v[k][j];
		}
	}
	memory_copy_2dim( (void**)pfm->v,(void**)pfm->vsort, pfm->N1, pfm->N, sizeof(double));
}


int __fminsearch_checkV(pFMinSearch pfm)
{
	int i,j;
	double t;
	double max = -MAX_DOUBLE;
	
	for ( i = 0 ; i < pfm->N  ; i++ )
	{
		for ( j = 0 ; j < pfm->N ; j++ )
		{
			t = fabs(pfm->v[i+1][j] - pfm->v[i][j] );
			if ( t > max ) max = t;
		}
	}
	return max <= pfm->tolx;
}

int __fminsearch_checkF(pFMinSearch pfm)
{
	int i;
	double t;
	double max = -MAX_DOUBLE;
	for ( i = 1 ; i < pfm->N1 ; i++ )
	{
		t = fabs( pfm->fv[i] - pfm->fv[0] );
		if ( t > max ) max = t;
	}
	return max <= pfm->tolf;
}

void __fminsearch_min_init(pFMinSearch pfm, double* X0)
{
	int i,j;
	for ( i = 0 ; i < pfm->N1 ; i++ )
	{
		for ( j = 0 ; j < pfm->N ; j++ )
		{
            if ( i > 1 && isinf(pfm->fv[i-1])) {
                if ( (i - 1)  == j )
                {
                    pfm->v[i][j] = X0[j] ? ( 1 + pfm->delta*100 ) * X0[j] : pfm->zero_delta;
                }
                else
                {
                    pfm->v[i][j] = X0[j];
                }                
            }
            else {
                if ( (i - 1)  == j )
                {
                    pfm->v[i][j] = X0[j] ? ( 1 + pfm->delta ) * X0[j] : pfm->zero_delta;
                }
                else
                {
                    pfm->v[i][j] = X0[j];
                }
            }
		}
		pfm->fv[i] = pfm->eq(pfm->v[i], pfm->args);
	}
	__fminsearch_sort(pfm);
}

void __fminsearch_x_mean(pFMinSearch pfm)
{
	int i,j;
	for ( i = 0 ; i < pfm->N ; i++ )
	{
		pfm->x_mean[i] = 0;
		for ( j = 0 ; j < pfm->N ; j++ )
		{
			pfm->x_mean[i] += pfm->v[j][i];
		}
		pfm->x_mean[i] /= pfm->N;
	}
}

double __fminsearch_x_reflection(pFMinSearch pfm)
{
	int i;
	for ( i = 0 ; i < pfm->N ; i++ )
	{
		pfm->x_r[i] = pfm->x_mean[i] + pfm->rho * ( pfm->x_mean[i] - pfm->v[pfm->N][i] );
	}
	return pfm->eq(pfm->x_r, pfm->args);
}


double __fminsearch_x_expansion(pFMinSearch pfm)
{
	int i;
	for ( i = 0 ; i < pfm->N ; i++ )
	{
		pfm->x_tmp[i] = pfm->x_mean[i] + pfm->chi * ( pfm->x_r[i] - pfm->x_mean[i] );
	}
	return pfm->eq(pfm->x_tmp, pfm->args);
}

double __fminsearch_x_contract_outside(pFMinSearch pfm)
{
	int i;
	for ( i = 0 ; i < pfm->N; i++ )
	{
		pfm->x_tmp[i] = pfm->x_mean[i] + pfm->psi * ( pfm->x_r[i] - pfm->x_mean[i] );
	}
	return pfm->eq(pfm->x_tmp, pfm->args);
}

double __fminsearch_x_contract_inside(pFMinSearch pfm)
{
	int i;
	for ( i = 0 ; i < pfm->N ; i++ )
	{
		pfm->x_tmp[i] = pfm->x_mean[i] + pfm->psi * ( pfm->x_mean[i] - pfm->v[pfm->N][i] );
	}
	return pfm->eq(pfm->x_tmp, pfm->args);
}

void __fminsearch_x_shrink(pFMinSearch pfm)
{
	int i, j;
	for ( i = 1 ; i < pfm->N1 ; i++ )
	{
		for ( j = 0 ; j < pfm->N ; j++ )
		{
			pfm->v[i][j] = pfm->v[0][j] + pfm->sigma * ( pfm->v[i][j] - pfm->v[0][j] );
		}
		pfm->fv[i] = pfm->eq(pfm->v[i], pfm->args);
	}
	__fminsearch_sort(pfm);
}

void __fminsearch_set_last_element(pFMinSearch pfm, double* x, double f)
{
	int i;
	for ( i = 0 ; i < pfm->N ; i++ )
	{
		pfm->v[pfm->N][i] = x[i];
	}
	pfm->fv[pfm->N] = f;
	__fminsearch_sort(pfm);
}

int fminsearch_min(pFMinSearch pfm, double* X0)
{
	int i;
	__fminsearch_min_init(pfm, X0);
	for ( i = 0 ; i < pfm->maxiters; i++ )
	{
		if ( __fminsearch_checkV(pfm) && __fminsearch_checkF(pfm) ) break;
        __fminsearch_x_mean(pfm);
		double fv_r = __fminsearch_x_reflection(pfm);
		if ( fv_r < pfm->fv[0] )
		{
			double fv_e = __fminsearch_x_expansion(pfm);
			if ( fv_e < fv_r ) __fminsearch_set_last_element(pfm,pfm->x_tmp, fv_e);
			else __fminsearch_set_last_element(pfm,pfm->x_r, fv_r);
		}
		else if ( fv_r >= pfm->fv[pfm->N] )
		{
			if ( fv_r > pfm->fv[pfm->N] )
			{
				double fv_cc = __fminsearch_x_contract_inside(pfm);
				if ( fv_cc < pfm->fv[pfm->N] ) __fminsearch_set_last_element(pfm,pfm->x_tmp, fv_cc);
				else __fminsearch_x_shrink(pfm);
			}
			else
			{
				double fv_c = __fminsearch_x_contract_outside(pfm);
				if ( fv_c <= fv_r ) __fminsearch_set_last_element(pfm,pfm->x_tmp, fv_c);
				else __fminsearch_x_shrink(pfm);
			}
		}
		else
		{
			__fminsearch_set_last_element(pfm,pfm->x_r, fv_r);
		}
	}
	pfm->bymax = i == pfm->maxiters;
	pfm->iters = i;
	return pfm->bymax;
}

double* fminsearch_get_minX(pFMinSearch pfm)
{
	return pfm->v[0];
}

double fminsearch_get_minF(pFMinSearch pfm)
{
	return pfm->fv[0];
}

