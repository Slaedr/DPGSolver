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

#include "compute_face_rlhs_dg.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "macros.h"
#include "definitions_intrusive.h"

#include "matrix.h"
#include "multiarray.h"
#include "vector.h"

#include "face_solver_dg.h"
#include "element_solver.h"
#include "volume.h"
#include "volume_solver_dg.h"

#include "compute_rlhs.h"
#include "compute_face_rlhs.h"
#include "multiarray_operator.h"
#include "numerical_flux.h"
#include "operator.h"
#include "simulation.h"
#include "solve_dg.h"
#include "test_case.h"

// Static function declarations ************************************************************************************* //

struct Num_Flux;

/** \brief Function pointer to the function used to scale by the face Jacobian.
 *
 *  \param num_flux \ref Numerical_Flux.
 *  \param face     \ref Face.
 *  \param sim      \ref Simulation.
 */
typedef void (*scale_by_Jacobian_fptr)
	(const struct Numerical_Flux* num_flux,
	 struct Face* face,
	 const struct Simulation* sim
	);

/** \brief Function pointer to the function used to evaluate the rhs (and optionally lhs) terms.
 *
 *  \param num_flux  \ref Numerical_Flux.
 *  \param dg_s_face \ref DG_Solver_Face.
 *  \param s_store_i \ref Solver_Storage_Implicit.
 *  \param sim       \ref Simulation.
 */
typedef void (*compute_rlhs_fptr)
	(const struct Numerical_Flux* num_flux,
	 struct DG_Solver_Face* dg_s_face,
	 struct Solver_Storage_Implicit* s_store_i,
	 const struct Simulation* sim
	);

/// \brief Container for solver-related parameters.
struct S_Params {
	scale_by_Jacobian_fptr scale_by_Jacobian; ///< Pointer to the appropriate function.

	compute_rlhs_fptr compute_rlhs; ///< Pointer to the appropriate function.
};

/// \brief Container for numerical flux related parameters.
struct Num_Flux {
	const struct const_Multiarray_d* n_dot_nf; ///< Unit normal dotted with the numerical flux.
};

/** \brief Constructor for the solution evaluated at the face cubature nodes.
 *  \return Standard. */
const struct const_Multiarray_d* constructor_sol_fc
	(struct Face* face,           ///< \ref Face.
	 const struct Simulation* sim ///< \ref Simulation.
	);

/** \brief Set the parameters of \ref S_Params.
 *  \return A statically allocated \ref S_Params container. */
static struct S_Params set_s_params
	(const struct Simulation* sim ///< \ref Simulation.
	);

// Interface functions ********************************************************************************************** //

void compute_face_rlhs_dg (const struct Simulation* sim, struct Solver_Storage_Implicit* s_store_i)
{
	assert(sim->elements->name == IL_ELEMENT_SOLVER_DG);
	assert(sim->faces->name    == IL_FACE_SOLVER_DG);
	assert(sim->volumes->name  == IL_VOLUME_SOLVER_DG);

	struct S_Params s_params = set_s_params(sim);
	struct Numerical_Flux_Input* num_flux_i = constructor_Numerical_Flux_Input(sim); // destructed

	for (struct Intrusive_Link* curr = sim->faces->first; curr; curr = curr->next) {
		struct Face* face                = (struct Face*) curr;
		struct Solver_Face* s_face       = (struct Solver_Face*) curr;
		struct DG_Solver_Face* dg_s_face = (struct DG_Solver_Face*) curr;
//printf("face: %d\n",face->index);

		constructor_Numerical_Flux_Input_data(num_flux_i,s_face,sim); // destructed
#if 0
print_const_Multiarray_d(num_flux_i->bv_l.s);
print_const_Multiarray_d(num_flux_i->bv_r.s);
#endif

		struct Numerical_Flux* num_flux = constructor_Numerical_Flux(num_flux_i); // destructed
		destructor_Numerical_Flux_Input_data(num_flux_i);
#if 0
print_const_Multiarray_d(num_flux->nnf);
print_const_Multiarray_d(num_flux->neigh_info[0].dnnf_ds);
if (!face->boundary)
	print_const_Multiarray_d(num_flux->neigh_info[1].dnnf_ds);
#endif

		s_params.scale_by_Jacobian(num_flux,face,sim);
#if 0
print_const_Multiarray_d(num_flux->nnf);
print_const_Multiarray_d(num_flux->neigh_info[0].dnnf_ds);
if (!face->boundary)
	print_const_Multiarray_d(num_flux->neigh_info[1].dnnf_ds);
#endif
		s_params.compute_rlhs(num_flux,dg_s_face,s_store_i,sim);
		destructor_Numerical_Flux(num_flux);
//if (face->index == 2)
//break;
//EXIT_UNSUPPORTED;
	}
//EXIT_UNSUPPORTED;
	destructor_Numerical_Flux_Input(num_flux_i);
}

// Static functions ************************************************************************************************* //
// Level 0 ********************************************************************************************************** //

/// \brief Scale \ref Numerical_Flux::nnf by the face Jacobian (i.e. only the explicit term).
static void scale_by_Jacobian_e
	(const struct Numerical_Flux* num_flux, ///< Defined for \ref scale_by_Jacobian_fptr.
	 struct Face* face,                     ///< Defined for \ref scale_by_Jacobian_fptr.
	 const struct Simulation* sim           ///< Defined for \ref scale_by_Jacobian_fptr.
	);

/// \brief Scale \ref Numerical_Flux::nnf and \ref Numerical_Flux::Neigh_Info_NF::dnnf_ds by the face Jacobian.
static void scale_by_Jacobian_i1
	(const struct Numerical_Flux* num_flux, ///< Defined for \ref scale_by_Jacobian_fptr.
	 struct Face* face,                     ///< Defined for \ref scale_by_Jacobian_fptr.
	 const struct Simulation* sim           ///< Defined for \ref scale_by_Jacobian_fptr.
	);

/// \brief Compute only the rhs term.
static void compute_rhs_f_dg
	(const struct Numerical_Flux* num_flux,     ///< Defined for \ref compute_rlhs_fptr.
	 struct DG_Solver_Face* dg_s_face,          ///< Defined for \ref compute_rlhs_fptr.
	 struct Solver_Storage_Implicit* s_store_i, ///< Defined for \ref compute_rlhs_fptr.
	 const struct Simulation* sim               ///< Defined for \ref compute_rlhs_fptr.
	);

/// \brief Compute the rhs and first order lhs terms.
static void compute_rlhs_1
	(const struct Numerical_Flux* num_flux,     ///< Defined for \ref compute_rlhs_fptr.
	 struct DG_Solver_Face* dg_s_face,          ///< Defined for \ref compute_rlhs_fptr.
	 struct Solver_Storage_Implicit* s_store_i, ///< Defined for \ref compute_rlhs_fptr.
	 const struct Simulation* sim               ///< Defined for \ref compute_rlhs_fptr.
	);

static struct S_Params set_s_params (const struct Simulation* sim)
{
	struct S_Params s_params;

	struct Test_Case* test_case = sim->test_case;
	switch (test_case->solver_method_curr) {
	case 'e':
		s_params.scale_by_Jacobian = scale_by_Jacobian_e;
		s_params.compute_rlhs      = compute_rhs_f_dg;
		break;
	case 'i':
		if (test_case->has_1st_order && !test_case->has_2nd_order) {
			s_params.scale_by_Jacobian = scale_by_Jacobian_i1;
			s_params.compute_rlhs      = compute_rlhs_1;
		} else if (!test_case->has_1st_order && test_case->has_2nd_order) {
			EXIT_ADD_SUPPORT; //s_params.compute_rlhs = compute_rlhs_2;
		} else if (test_case->has_1st_order && test_case->has_2nd_order) {
			EXIT_ADD_SUPPORT; //s_params.compute_rlhs = compute_rlhs_12;
		} else {
			EXIT_ERROR("Unsupported: %d %d\n",test_case->has_1st_order,test_case->has_2nd_order);
		}
		break;
	default:
		EXIT_ERROR("Unsupported: %c\n",test_case->solver_method_curr);
		break;
	}

	return s_params;
}

// Level 1 ********************************************************************************************************** //

/// \brief Finalize the rhs term contribution from the \ref Face.
static void finalize_face_rhs_dg
	(const int side_index,                  ///< The index of the side of the face under consideration.
	 const struct Numerical_Flux* num_flux, ///< Defined for \ref compute_rlhs_fptr.
	 struct DG_Solver_Face* dg_s_face,      ///< Defined for \ref compute_rlhs_fptr.
	 const struct Simulation* sim           ///< Defined for \ref compute_rlhs_fptr.
	);

/// \brief Finalize the lhs term contribution from the \ref Face for the dg scheme.
static void finalize_lhs_f_dg
	(const int side_index[2],                  /**< The indices of the affectee, affector, respectively. See the
	                                            *   comments in \ref compute_face_rlhs_dg.h for the convention. */
	 const struct Numerical_Flux* num_flux,    ///< Defined for \ref compute_rlhs_fptr.
	 struct DG_Solver_Face* dg_s_face,         ///< Defined for \ref compute_rlhs_fptr.
	 struct Solver_Storage_Implicit* s_store_i ///< Defined for \ref compute_rlhs_fptr.
	);

static void scale_by_Jacobian_e (const struct Numerical_Flux* num_flux, struct Face* face, const struct Simulation* sim)
{
UNUSED(sim);
	struct Solver_Face* s_face = (struct Solver_Face*)face;

	const struct const_Vector_d jacobian_det_fc = interpret_const_Multiarray_as_Vector_d(s_face->jacobian_det_fc);
	scale_Multiarray_by_Vector_d('L',1.0,(struct Multiarray_d*)num_flux->nnf,&jacobian_det_fc,false);
}

static void scale_by_Jacobian_i1
	(const struct Numerical_Flux* num_flux, struct Face* face, const struct Simulation* sim)
{
UNUSED(sim);
	assert((!face->boundary && num_flux->neigh_info[1].dnnf_ds != NULL) ||
	       ( face->boundary && num_flux->neigh_info[1].dnnf_ds == NULL));

	struct Solver_Face* s_face = (struct Solver_Face*)face;

	const struct const_Vector_d jacobian_det_fc = interpret_const_Multiarray_as_Vector_d(s_face->jacobian_det_fc);
	scale_Multiarray_by_Vector_d('L',1.0,(struct Multiarray_d*)num_flux->nnf,&jacobian_det_fc,false);
	scale_Multiarray_by_Vector_d('L',1.0,(struct Multiarray_d*)num_flux->neigh_info[0].dnnf_ds,&jacobian_det_fc,false);
	if (!face->boundary)
		scale_Multiarray_by_Vector_d('L',1.0,(struct Multiarray_d*)num_flux->neigh_info[1].dnnf_ds,&jacobian_det_fc,false);
}

static void compute_rhs_f_dg
	(const struct Numerical_Flux* num_flux, struct DG_Solver_Face* dg_s_face,
	 struct Solver_Storage_Implicit* s_store_i, const struct Simulation* sim)
{
	UNUSED(s_store_i);
	assert(sim->elements->name == IL_ELEMENT_SOLVER_DG);

	const struct Face* face = (struct Face*) dg_s_face;
	finalize_face_rhs_dg(0,num_flux,dg_s_face,sim);
	if (!face->boundary) {
		permute_Multiarray_d_fc((struct Multiarray_d*)num_flux->nnf,'R',1,(struct Solver_Face*)face);
// Can remove both of the scalings (here and and finalize_face_rhs_dg).
		scale_Multiarray_d((struct Multiarray_d*)num_flux->nnf,-1.0); // Use "-ve" normal.
		finalize_face_rhs_dg(1,num_flux,dg_s_face,sim);
	}
}

static void compute_rlhs_1
	(const struct Numerical_Flux* num_flux, struct DG_Solver_Face* dg_s_face,
	 struct Solver_Storage_Implicit* s_store_i, const struct Simulation* sim)
{
	const struct Face* face = (struct Face*) dg_s_face;

	/// See \ref compute_face_rlhs_dg.h for the `lhs_**` notation.
	compute_rhs_f_dg(num_flux,dg_s_face,s_store_i,sim);

	finalize_lhs_f_dg((int[]){0,0},num_flux,dg_s_face,s_store_i); // lhs_ll
//EXIT_UNSUPPORTED;
	if (!face->boundary) {
//printf("\n\n\n");
		finalize_lhs_f_dg((int[]){0,1},num_flux,dg_s_face,s_store_i); // lhs_lr

		for (int i = 0; i < 2; ++i) {
			const struct Neigh_Info_NF* n_i = &num_flux->neigh_info[i];
			permute_Multiarray_d_fc((struct Multiarray_d*)n_i->dnnf_ds,'R',1,(struct Solver_Face*)face);
			scale_Multiarray_d((struct Multiarray_d*)n_i->dnnf_ds,-1.0); // Use "-ve" normal.
		}

		finalize_lhs_f_dg((int[]){1,0},num_flux,dg_s_face,s_store_i); // lhs_rl
		finalize_lhs_f_dg((int[]){1,1},num_flux,dg_s_face,s_store_i); // lhs_rr
//EXIT_UNSUPPORTED;
	}
}

// Level 2 ********************************************************************************************************** //

static void finalize_face_rhs_dg
	(const int side_index, const struct Numerical_Flux* num_flux, struct DG_Solver_Face* dg_s_face,
	 const struct Simulation* sim)
{
	const struct Face* face          = (struct Face*) dg_s_face;
	const struct Solver_Face* s_face = (struct Solver_Face*) face;
	const struct Operator* tw0_vt_fc = get_operator__tw0_vt_fc(side_index,s_face);

UNUSED(sim);
	// sim may be used to store a parameter establishing which type of operator to use for the computation.
	const char op_format = 'd';

	struct DG_Solver_Volume* dg_s_vol = (struct DG_Solver_Volume*) face->neigh_info[side_index].volume;

//printf("%d\n",vol->index);
	mm_NNC_Operator_Multiarray_d(-1.0,1.0,tw0_vt_fc,num_flux->nnf,dg_s_vol->rhs,op_format,2,NULL,NULL);
//print_Multiarray_d(dg_s_vol->rhs);
}

static void finalize_lhs_f_dg
	(const int side_index[2], const struct Numerical_Flux* num_flux, struct DG_Solver_Face* dg_s_face,
	 struct Solver_Storage_Implicit* s_store_i)
{
	struct Face* face              = (struct Face*) dg_s_face;
	struct Solver_Face* s_face     = (struct Solver_Face*) face;

	struct Matrix_d* lhs = constructor_lhs_f_1(side_index,num_flux,s_face); // destructed

	struct Solver_Volume* s_vol[2] = { (struct Solver_Volume*) face->neigh_info[0].volume,
	                                   (struct Solver_Volume*) face->neigh_info[1].volume, };
//printf("%td %td\n",s_vol[side_index[0]]->ind_dof,s_vol[side_index[1]]->ind_dof);
	set_petsc_Mat_row_col(s_store_i,s_vol[side_index[0]],0,s_vol[side_index[1]],0);
	add_to_petsc_Mat(s_store_i,(struct const_Matrix_d*)lhs);

	destructor_Matrix_d(lhs);
}
