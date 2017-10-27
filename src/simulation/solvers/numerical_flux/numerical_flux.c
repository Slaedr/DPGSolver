/* {{{
This file is part of DPGSolver.

DPGSolver is free software: you can redistribute it and/or modify it under the terms of the GNU
General Public License as published by the Free Software Foundation, either version 3 of the
License, or any later version.

DPGSolver is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
Public License for more details.

You should have received a copy of the GNU General Public License along with DPGSolver.  If not, see
<http://www.gnu.org/licenses/>.
}}} */
/** \file
 */

#include "numerical_flux.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "macros.h"

#include "multiarray.h"
#include "vector.h"

#include "element_solver_dg.h"
#include "face.h"
#include "face_solver.h"
#include "volume.h"
#include "volume_solver.h"

#include "const_cast.h"
#include "multiarray_operator.h"
#include "operator.h"
#include "simulation.h"
#include "solve_dg.h"
#include "test_case.h"

// Static function declarations ************************************************************************************* //

// Interface functions ********************************************************************************************** //

struct Numerical_Flux_Input* constructor_Numerical_Flux_Input (const struct Simulation* sim)
{
	struct Numerical_Flux_Input* num_flux_i = calloc(1,sizeof *num_flux_i); // returned

	const_cast_c1(&num_flux_i->bv_l.input_path,sim->input_path);

	struct Test_Case* test_case = sim->test_case;
	const_cast_b(&num_flux_i->has_1st_order,test_case->has_1st_order);
	const_cast_b(&num_flux_i->has_2nd_order,test_case->has_2nd_order);

	const_cast_i(&num_flux_i->bv_l.d,sim->d);
	const_cast_i(&num_flux_i->bv_l.n_eq,test_case->n_eq);
	const_cast_i(&num_flux_i->bv_l.n_var,test_case->n_var);

	num_flux_i->compute_Numerical_Flux = test_case->compute_Numerical_Flux;
	switch (test_case->solver_method_curr) {
	case 'e':
		num_flux_i->bv_l.compute_member = test_case->flux_comp_mem_e;
		num_flux_i->compute_Numerical_Flux_1st = test_case->compute_Numerical_Flux_e[0];
		num_flux_i->compute_Numerical_Flux_2nd = test_case->compute_Numerical_Flux_e[1];
		break;
	case 'i':
		num_flux_i->bv_l.compute_member = test_case->flux_comp_mem_i;
		num_flux_i->compute_Numerical_Flux_1st = test_case->compute_Numerical_Flux_i[0];
		num_flux_i->compute_Numerical_Flux_2nd = test_case->compute_Numerical_Flux_i[1];
		break;
	default:
		EXIT_ERROR("Unsupported: %c.\n",test_case->solver_method_curr);
		break;
	}

	return num_flux_i;
}

void destructor_Numerical_Flux_Input (struct Numerical_Flux_Input* num_flux_i)
{
	free(num_flux_i);
}

void destructor_Numerical_Flux_Input_mem (struct Numerical_Flux_Input* num_flux_i)
{
	if (num_flux_i->bv_l.s)
		destructor_const_Multiarray_d(num_flux_i->bv_l.s);
	if (num_flux_i->bv_r.s)
		destructor_const_Multiarray_d(num_flux_i->bv_r.s);
	if (num_flux_i->bv_l.g)
		destructor_const_Multiarray_d(num_flux_i->bv_l.g);
	if (num_flux_i->bv_r.g)
		destructor_const_Multiarray_d(num_flux_i->bv_r.g);
}

struct Numerical_Flux* constructor_Numerical_Flux (const struct Numerical_Flux_Input* num_flux_i)
{
	assert(((num_flux_i->bv_l.s != NULL && num_flux_i->bv_l.s->layout == 'C') &&
	        (num_flux_i->bv_r.s != NULL && num_flux_i->bv_r.s->layout == 'C')) ||
	       ((num_flux_i->bv_l.g != NULL && num_flux_i->bv_l.g->layout == 'C') &&
	        (num_flux_i->bv_r.g != NULL && num_flux_i->bv_r.g->layout == 'C')));

	const bool* c_m = num_flux_i->bv_l.compute_member;
	assert(c_m[0] || c_m[1] || c_m[2]);

	const int d     = num_flux_i->bv_l.d,
	          n_eq  = num_flux_i->bv_l.n_eq,
		    n_var = num_flux_i->bv_l.n_var;
	const ptrdiff_t n_n = ( num_flux_i->bv_l.s != NULL ? num_flux_i->bv_l.s->extents[0]
	                                                   : num_flux_i->bv_l.g->extents[0] );

	struct mutable_Numerical_Flux* num_flux = calloc(1,sizeof *num_flux); // returned

	num_flux->nnf = (c_m[0] ? constructor_zero_Multiarray_d('C',2,(ptrdiff_t[]){n_n,n_eq}) : NULL);
	for (int i = 0; i < 2; ++i) {
		struct m_Neigh_Info_NF* n_i = &num_flux->neigh_info[i];
		n_i->dnnf_ds = (c_m[1] ? constructor_zero_Multiarray_d('C',3,(ptrdiff_t[]){n_n,n_eq,n_var})   : NULL);
		n_i->dnnf_dg = (c_m[2] ? constructor_zero_Multiarray_d('C',4,(ptrdiff_t[]){n_n,n_eq,n_var,d}) : NULL);
	}

	num_flux_i->compute_Numerical_Flux(num_flux_i,num_flux);

	return (struct Numerical_Flux*) num_flux;
}

void destructor_Numerical_Flux (struct Numerical_Flux* num_flux)
{
	if (num_flux->nnf != NULL)
		destructor_const_Multiarray_d(num_flux->nnf);

	for (int i = 0; i < 2; ++i) {
		struct Neigh_Info_NF n_i = num_flux->neigh_info[i];
		if (n_i.dnnf_ds != NULL)
			destructor_const_Multiarray_d(n_i.dnnf_ds);
		if (n_i.dnnf_dg != NULL)
			destructor_const_Multiarray_d(n_i.dnnf_dg);
	}
	free(num_flux);
}

void compute_Numerical_Flux_1 (const struct Numerical_Flux_Input* num_flux_i, struct mutable_Numerical_Flux* num_flux)
{
	num_flux_i->compute_Numerical_Flux_1st(num_flux_i,num_flux);
}

void compute_Numerical_Flux_12 (const struct Numerical_Flux_Input* num_flux_i, struct mutable_Numerical_Flux* num_flux)
{
	num_flux_i->compute_Numerical_Flux_1st(num_flux_i,num_flux);
	num_flux_i->compute_Numerical_Flux_2nd(num_flux_i,num_flux);
}

// Static functions ************************************************************************************************* //
// Level 0 ********************************************************************************************************** //
