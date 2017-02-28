#ifndef VITERBI_H_A08989A1_B4B4_461C_B863_A1AE2FE9BD98
#define VITERBI_H_A08989A1_B4B4_461C_B863_A1AE2FE9BD98

extern "C"
{
#include <family.h>
void compute_viterbis(pCafeNode node, int k, double *factors, int rootfamilysize_start, int rootfamilysize_end, int familysize_start, int familysize_end);
}

void viterbi_parameters_init(viterbi_parameters *viterbi, int nnodes, int nrows);

void viterbi_set_max_pvalue(viterbi_parameters* viterbi, int index, double val);
void viterbi_parameters_clear(viterbi_parameters* viterbi, int nnodes);


#endif
