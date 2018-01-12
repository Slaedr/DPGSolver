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

#include <assert.h>
#include <string.h>

#include "macros.h"
#include "definitions_test_case.h"
#include "definitions_bc.h"

#include "def_templates_face_solver.h"
#include "def_templates_volume_solver.h"

#include "def_templates_multiarray.h"
#include "def_templates_vector.h"

#include "def_templates_boundary.h"
#include "def_templates_test_case.h"

// Static function declarations ************************************************************************************* //

/** \brief Checks if one of the neighbouring volumes to the current face is curved.
 *  \return `true` if curved volume is found; `false` otherwise. */
static bool check_for_curved_neigh
	(struct Face* face ///< \ref Face.
	);

/** \brief Set the function pointers to the appropriate functions to compute boundary values needed for the numerical
 *         flux computation. */
static void set_function_pointers_num_flux_bc_T
	(struct Solver_Face_T* s_face, ///< Defined for \ref set_function_pointers_face_num_flux_T.
	 const struct Simulation* sim  ///< Defined for \ref set_function_pointers_face_num_flux_T.
	);

// Interface functions ********************************************************************************************** //

void constructor_derived_Solver_Face_T (struct Face* face_ptr, const struct Simulation* sim)
{
	struct Solver_Face_T* s_face = (struct Solver_Face_T*) face_ptr;

	const_cast_ptrdiff(&s_face->ind_dof,-1);
	const_cast_i(&s_face->p_ref,sim->p_ref[0]);
	const_cast_i(&s_face->ml,0);
	const_cast_c(&s_face->cub_type,(check_for_curved_neigh((struct Face*)s_face) ? 'c' : 's'));

	s_face->nf_coef = constructor_empty_Multiarray_T('C',2,(ptrdiff_t[]){0,0});   // destructed

	const_constructor_move_Multiarray_R(
		&s_face->xyz_fc,constructor_empty_Multiarray_R('C',2,(ptrdiff_t[]){0,0}));        // destructed
	const_constructor_move_Multiarray_R(
		&s_face->normals_fc,constructor_empty_Multiarray_R('R',2,(ptrdiff_t[]){0,0}));    // destructed
	const_constructor_move_Multiarray_R(
		&s_face->jacobian_det_fc,constructor_empty_Multiarray_R('C',1,(ptrdiff_t[]){0})); // destructed

	set_function_pointers_face_num_flux_T(s_face,sim);

	s_face->nf_fc = NULL;
}

void destructor_derived_Solver_Face_T (struct Face* face_ptr)
{
	struct Solver_Face_T* face = (struct Solver_Face_T*) face_ptr;

	destructor_Multiarray_T(face->nf_coef);

	destructor_const_Multiarray_R(face->xyz_fc);
	destructor_const_Multiarray_R(face->normals_fc);
	destructor_const_Multiarray_R(face->jacobian_det_fc);

	destructor_conditional_const_Multiarray_T(face->nf_fc);
}

void set_function_pointers_face_num_flux_T (struct Solver_Face_T* s_face, const struct Simulation* sim)
{
	const struct Face* face = (struct Face*) s_face;
	if (!face->boundary) {
		struct Test_Case_T* test_case = (struct Test_Case_T*)sim->test_case_rc->tc;
		switch (test_case->pde_index) {
		case PDE_ADVECTION:
		case PDE_EULER:
			s_face->constructor_Boundary_Value_fcl = constructor_Boundary_Value_s_fcl_interp_T;
			break;
		case PDE_POISSON:
			EXIT_UNSUPPORTED;
			break;
		case PDE_NAVIER_STOKES:
			EXIT_UNSUPPORTED;
			break;
		default:
			EXIT_ERROR("Unsupported: %d\n",test_case->pde_index);
			break;
		}
	} else {
		set_function_pointers_num_flux_bc_T(s_face,sim);
	}
}

const struct const_Vector_R* get_operator__w_fc__s_e_T (const struct Solver_Face_T*const s_face)
{
	const int side_index = 0;
	const struct Face*const face          = (struct Face*) s_face;
	const struct Volume*const vol         = face->neigh_info[side_index].volume;
	const struct Solver_Element*const s_e = (struct Solver_Element*) vol->element;

	const int p_f = s_face->p_ref;

	const int curved = ( (s_face->cub_type == 's') ? 0 : 1 );
	return get_const_Multiarray_Vector_d(s_e->w_fc[curved],(ptrdiff_t[]){0,0,0,0,p_f,p_f});
}

// Static functions ************************************************************************************************* //
// Level 0 ********************************************************************************************************** //

/// \brief Version of \ref set_function_pointers_num_flux_bc_T for the linear advection equation.
static void set_function_pointers_num_flux_bc_advection_T
	(struct Solver_Face_T* s_face ///< See brief.
	);

/// \brief Version of \ref set_function_pointers_num_flux_bc_T for the Euler equations.
static void set_function_pointers_num_flux_bc_euler_T
	(struct Solver_Face_T* s_face ///< See brief.
	);

static bool check_for_curved_neigh (struct Face* face)
{
	if (face->neigh_info[0].volume->curved || (face->neigh_info[1].volume && face->neigh_info[1].volume->curved))
		return true;
	return false;
}

static void set_function_pointers_num_flux_bc_T (struct Solver_Face_T* s_face, const struct Simulation* sim)
{
	struct Test_Case_T* test_case = (struct Test_Case_T*)sim->test_case_rc->tc;
	switch (test_case->pde_index) {
	case PDE_ADVECTION: set_function_pointers_num_flux_bc_advection_T(s_face); break;
	case PDE_EULER:     set_function_pointers_num_flux_bc_euler_T(s_face);     break;
	default:
		EXIT_ERROR("Unsupported: %d\n",test_case->pde_index);
		break;
	}
}

// Level 1 ********************************************************************************************************** //

static void set_function_pointers_num_flux_bc_advection_T (struct Solver_Face_T* s_face)
{
	const struct Face* face = (struct Face*) s_face;

	const int bc = face->bc % BC_STEP_SC;
	switch (bc) {
	case BC_INFLOW:
		s_face->constructor_Boundary_Value_fcl = constructor_Boundary_Value_T_advection_inflow;
		break;
	case BC_OUTFLOW:
		s_face->constructor_Boundary_Value_fcl = constructor_Boundary_Value_T_advection_outflow;
		break;
	default:
		EXIT_ERROR("Unsupported: %d\n",face->bc);
		break;
	}
}

static void set_function_pointers_num_flux_bc_euler_T (struct Solver_Face_T* s_face)
{
	const struct Face* face = (struct Face*) s_face;

	const int bc = face->bc % BC_STEP_SC;
	switch (bc) {
	case BC_RIEMANN:
		s_face->constructor_Boundary_Value_fcl = constructor_Boundary_Value_T_euler_riemann;
		break;
	case BC_SLIPWALL:
		s_face->constructor_Boundary_Value_fcl = constructor_Boundary_Value_T_euler_slipwall;
		break;
	case BC_SUPERSONIC_IN:
		s_face->constructor_Boundary_Value_fcl = constructor_Boundary_Value_T_euler_supersonic_inflow;
		break;
	case BC_SUPERSONIC_OUT:
		s_face->constructor_Boundary_Value_fcl = constructor_Boundary_Value_T_euler_supersonic_outflow;
		break;
	case BC_BACKPRESSURE:
	case BC_TOTAL_TP:
	default:
		EXIT_ERROR("Unsupported: %d\n",face->bc);
		break;
	}
}
