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

#include "compute_face_rlhs_opg.h"

#include "definitions_test_case.h"

#include "matrix.h"
#include "multiarray.h"
#include "vector.h"

#include "face_solver_opg.h"
#include "element_solver_opg.h"
#include "volume.h"
#include "volume_solver_opg.h"

#include "computational_elements.h"
#include "compute_rlhs.h"
#include "compute_face_rlhs.h"
#include "multiarray_operator.h"
#include "numerical_flux.h"
#include "operator.h"
#include "simulation.h"
#include "solve.h"
#include "solve_opg.h"
#include "test_case.h"

// Static function declarations ************************************************************************************* //

/** Flag for whether the normal flux on the boundary faces should be updated. At the time of this implementation,
 *  \ref Solver_Face_T::nf_coef was not stored on the boundary faces as boundary normal fluxes were computed using the
 *  numerical flux; it may consequently be required to initialize nf_coef on boundary faces if enabling this option. */
#define UPDATE_NF_BOUNDARY 0

/// \brief Version of \ref compute_rlhs_f_fptr_T computing the rhs and lhs terms for 1st order equations only.
static void compute_rlhs_1
	(const struct Numerical_Flux*const num_flux, ///< See brief.
	 struct Solver_Face*const s_face,            ///< See brief.
	 struct Solver_Storage_Implicit*const ssi    ///< See brief.
	);

/** \brief Return the jump of the solution test functions, [[v]], at the face cubature nodes.
 *  \return See brief.
 *
 *  The jump is defined by:
 *  - internal faces: [[v]] := v_l - v_r;
 *  - boundary faces: [[v]] := v_l;
 */
static const struct const_Multiarray_d* constructor_jump_test_s_fc
	(const struct Solver_Face*const s_face ///< Standard.
	);

// Interface functions ********************************************************************************************** //

#include "def_templates_type_d.h"
#include "compute_face_rlhs_opg_T.c"

void update_coef_nf_f_opg (const struct Simulation*const sim)
{
	/** The L2 projection is current being used to project the test to the normal flux coefficients. For face
	 *  collocated schemes when the normal flux polynomial and test function polynomial degrees are equal, the
	 *  projection operator reduces to identity. */
	for (struct Intrusive_Link* curr = sim->faces->first; curr; curr = curr->next) {
		const struct Face*const face                  = (struct Face*) curr;
		if (!UPDATE_NF_BOUNDARY && face->boundary)
			continue;

		const struct Solver_Face*const s_face         = (struct Solver_Face*) curr;
		const struct OPG_Solver_Face*const opg_s_face = (struct OPG_Solver_Face*) curr;

		const struct Operator*const cv0_ff_fc = get_operator__cv0_ff_fc(0,s_face);
		const struct const_Matrix_d*const op_proj_L2 =
			constructor_mm_const_Matrix_d('N','T',1.0,opg_s_face->m_inv,cv0_ff_fc->op_std,'R'); // destructed

		const struct const_Vector_R*const w_fc  = get_operator__w_fc__s_e_T(s_face);
		const struct const_Vector_R j_det_fc    = interpret_const_Multiarray_as_Vector_R(s_face->jacobian_det_fc);
		const struct const_Vector_R*const wJ_fc = constructor_dot_mult_const_Vector_R(1.0,w_fc,&j_det_fc,1); // d.

		const struct const_Multiarray_d*const jump_test_s_fc = constructor_jump_test_s_fc(s_face); // destructed
		scale_Multiarray_by_Vector_d('L',1.0,(struct Multiarray_d*)jump_test_s_fc,wJ_fc,false);
		destructor_const_Vector_d(wJ_fc);

		mm_NNC_Multiarray_d(1.0,0.0,op_proj_L2,jump_test_s_fc,s_face->nf_coef);
		destructor_const_Matrix_d(op_proj_L2);
		destructor_const_Multiarray_d(jump_test_s_fc);
	}
}

// Static functions ************************************************************************************************* //
// Level 0 ********************************************************************************************************** //

/** \brief Version of \ref compute_rlhs_f_fptr_T computing the lhs terms for 1st order equations only for internal
 *         faces. */
static void compute_lhs_1_i
	(struct Solver_Face*const s_face,         ///< See brief.
	 struct Solver_Storage_Implicit*const ssi ///< See brief.
	);

/** \brief Version of \ref compute_rlhs_f_fptr_T computing the lhs terms for 1st order equations only for boundary
 *         faces. */
static void compute_lhs_1_b
	(const struct Numerical_Flux*const num_flux, ///< See brief.
	 struct Solver_Face*const s_face,            ///< See brief.
	 struct Solver_Storage_Implicit*const ssi    ///< See brief.
	);

/** \brief Get the pointer to the appropriate \ref OPG_Solver_Element::cv0_vt_fc operator.
 *  \return See brief. */
static const struct Operator* get_operator__cv0_vt_fc
	(const int side_index,                 ///< The index of the side of the face under consideration.
	 const struct Solver_Face*const s_face ///< The current \ref Face.
	);

static void compute_rlhs_1
	(const struct Numerical_Flux*const num_flux, struct Solver_Face*const s_face,
	 struct Solver_Storage_Implicit*const ssi)
{
	compute_rhs_f_dg_like(num_flux,s_face,ssi);

	const struct Face*const face = (struct Face*) s_face;
	if (!face->boundary)
		compute_lhs_1_i(s_face,ssi);
	else
		compute_lhs_1_b(num_flux,s_face,ssi);
}

static const struct const_Multiarray_d* constructor_jump_test_s_fc (const struct Solver_Face*const s_face)
{
	const struct Face*const face = (struct Face*) s_face;
	const struct const_Matrix_d* cv0_vt_fc[2] = { get_operator__cv0_vt_fc(0,s_face)->op_std, NULL, };
	const struct Solver_Volume* s_vol[2]      = { (struct Solver_Volume*) face->neigh_info[0].volume, NULL, };

	const struct const_Multiarray_d*const test_s_coef = (struct const_Multiarray_d*) s_vol[0]->test_s_coef;
	struct Multiarray_d*const jump_test_s = constructor_mm_NN1C_Multiarray_d(cv0_vt_fc[0],test_s_coef); // returned

	if (!face->boundary) {
		cv0_vt_fc[1] = constructor_copy_const_Matrix_d(get_operator__cv0_vt_fc(1,s_face)->op_std); // destructed
		permute_Matrix_d_fc((struct Matrix_d*)cv0_vt_fc[1],'R',0,s_face);
		s_vol[1] = (struct Solver_Volume*) face->neigh_info[1].volume;

		const struct const_Multiarray_d*const test_s_coef = (struct const_Multiarray_d*) s_vol[1]->test_s_coef;
		const struct const_Multiarray_d*const jump_test_s_R =
			constructor_mm_NN1C_const_Multiarray_d(cv0_vt_fc[1],test_s_coef); // destructed
		destructor_const_Matrix_d(cv0_vt_fc[1]);

		add_in_place_Multiarray_d(-1.0,jump_test_s,jump_test_s_R);
		destructor_const_Multiarray_d(jump_test_s_R);
	}

	return (struct const_Multiarray_d*) jump_test_s;
}

// Level 1 ********************************************************************************************************** //

/// \brief Container for operators needed for the assembly of LHS terms for the OPG scheme.
struct Lhs_Operators_OPG {
	const struct const_Vector_d* wJ_fc; ///< Face cubature weights "dot-multiplied" by the Jacobian determinant.

	/** 'c'oefficient to 'v'alue operators from the 'v'olume 't'est basis to 'f'ace 'c'ubature nodes.
	 *
	 *  The indices are used to denote the following operators:
	 *  - [0]: ll operator ('l'eft  basis -> 'l'eft  nodes);
	 *  - [1]: rl operator ('r'ight basis -> 'l'eft  nodes; permutation of node ordering);
	 */
	const struct const_Matrix_d* cv0_vt_fc[2];
};

/** \brief Constructor for a \ref Lhs_Operators_OPG container.
 *  \return See brief. */
static const struct Lhs_Operators_OPG* constructor_Lhs_Operators_OPG
	(const struct Solver_Face*const s_face ///< Standard.
	);

/// \brief Destructor for a \ref Lhs_Operators_OPG container.
static void destructor_Lhs_Operators_OPG
	(const struct Solver_Face*const s_face, ///< Standard.
	 const struct Lhs_Operators_OPG* ops    ///< Standard.
	);

/// \brief Finalize the 1st order lhs term contribution from the \ref Face for the opg scheme.
static void finalize_lhs_1_f_opg
	(const int side_index[2],                  /**< The indices of the affectee, affector, respectively. See the
	                                            *   comments in \ref compute_face_rlhs_dg.h for the convention. */
	 const struct Lhs_Operators_OPG*const ops, ///< Standard.
	 const struct Solver_Face*const s_face,    ///< Standard.
	 struct Solver_Storage_Implicit*const ssi  ///< Standard.
	);

/** \brief Constructor for the lhs term arising from the OPG scheme for boundary faces.
 *  \return See brief.
 *
 *  This term is nearly identical to that for the DG scheme with the modification being that the linearization is with
 *  respect to the test function coefficients and not the solution coefficients.
 */
const struct const_Matrix_d* constructor_lhs_f_1_b
	(const struct Numerical_Flux*const num_flux, ///< Standard.
	 const struct Solver_Face*const s_face       ///< Standard.
	);

static void compute_lhs_1_i (struct Solver_Face*const s_face, struct Solver_Storage_Implicit*const ssi)
{
	/// See \ref compute_face_rlhs_dg.h for the `lhs_**` notation.
	const struct Face*const face = (struct Face*) s_face;
	assert(!face->boundary);

	const struct Lhs_Operators_OPG*const ops = constructor_Lhs_Operators_OPG(s_face); // destructed

	finalize_lhs_1_f_opg((int[]){0,0},ops,s_face,ssi); // lhs_ll
	finalize_lhs_1_f_opg((int[]){0,1},ops,s_face,ssi); // lhs_lr
	finalize_lhs_1_f_opg((int[]){1,0},ops,s_face,ssi); // lhs_rl
	finalize_lhs_1_f_opg((int[]){1,1},ops,s_face,ssi); // lhs_rr

	destructor_Lhs_Operators_OPG(s_face,ops);
}

static void compute_lhs_1_b
	(const struct Numerical_Flux*const num_flux, struct Solver_Face*const s_face,
	 struct Solver_Storage_Implicit*const ssi)
{
	const struct Face*const face = (struct Face*) s_face;
	assert(face->boundary);

	const struct const_Matrix_d*const lhs = constructor_lhs_f_1_b(num_flux,s_face); // destructed

	const struct OPG_Solver_Volume*const opg_s_vol = (struct OPG_Solver_Volume*) face->neigh_info[0].volume;
	set_petsc_Mat_row_col_opg(ssi,opg_s_vol,0,opg_s_vol,0);
	add_to_petsc_Mat(ssi,lhs);

	destructor_const_Matrix_d(lhs);
}

static const struct Operator* get_operator__cv0_vt_fc (const int side_index, const struct Solver_Face*const s_face)
{
	const struct Face*const face            = (struct Face*) s_face;
	const struct Volume*const vol           = face->neigh_info[side_index].volume;
	const struct Solver_Volume*const s_vol  = (struct Solver_Volume*) vol;
	const struct OPG_Solver_Element*const e = (struct OPG_Solver_Element*) vol->element;

	const int ind_lf   = face->neigh_info[side_index].ind_lf,
	          ind_href = face->neigh_info[side_index].ind_href;
	const int p_v = s_vol->p_ref,
	          p_f = s_face->p_ref;

	const int curved = ( (s_face->cub_type == 's') ? 0 : 1 );
	return get_Multiarray_Operator(e->cv0_vt_fc[curved],(ptrdiff_t[]){ind_lf,ind_href,0,p_f,p_v});
}

// Level 2 ********************************************************************************************************** //

/** \brief Constructor for the 'l'eft term of the lhs contribution from the boundary faces.
 *  \return The operator corresponding to test_s' \frac{dnnf_ds}. */
static struct const_Matrix_d* constructor_lhs_f_1_b_l
	(const struct Numerical_Flux*const num_flux, ///< Standard.
	 const struct Solver_Face*const s_face       ///< Standard.
	);

/** \brief Constructor for the 'r'ight term of the lhs contribution from the boundary faces.
 *  \return The operator corresponding to \frac{d s}{d test_s_coef} = - df_ds' (dot) cv1_vt_fc. */
static struct const_Matrix_d* constructor_lhs_f_1_b_r
	(const struct Solver_Face*const s_face ///< Standard.
	);

static const struct Lhs_Operators_OPG* constructor_Lhs_Operators_OPG (const struct Solver_Face*const s_face)
{
	struct Lhs_Operators_OPG*const ops = calloc(1,sizeof *ops); // free

	const struct const_Vector_d*const w_fc  = get_operator__w_fc__s_e(s_face);
	const struct const_Vector_d j_det_fc    = interpret_const_Multiarray_as_Vector_d(s_face->jacobian_det_fc);
	ops->wJ_fc = constructor_dot_mult_const_Vector_d(1.0,w_fc,&j_det_fc,1); // destructed

	const struct Operator*const cv0_vt_fc = get_operator__cv0_vt_fc(0,s_face);
	ops->cv0_vt_fc[0] = cv0_vt_fc->op_std;

	const struct Face*const face = (struct Face*) s_face;
	if (!face->boundary) {
		ops->cv0_vt_fc[1] = constructor_copy_const_Matrix_d(get_operator__cv0_vt_fc(1,s_face)->op_std); // dest.
		permute_Matrix_d_fc((struct Matrix_d*)ops->cv0_vt_fc[1],'R',0,s_face);
	}
	return ops;
}

static void destructor_Lhs_Operators_OPG (const struct Solver_Face*const s_face, const struct Lhs_Operators_OPG* ops)
{
	destructor_const_Vector_d(ops->wJ_fc);

	const struct Face*const face = (struct Face*) s_face;
	if (!face->boundary)
		destructor_const_Matrix_d(ops->cv0_vt_fc[1]);
	free((void*)ops);
}

static void finalize_lhs_1_f_opg
	(const int side_index[2], const struct Lhs_Operators_OPG*const ops, const struct Solver_Face*const s_face,
	 struct Solver_Storage_Implicit*const ssi)
{
	const struct Face*const face = (struct Face*) s_face;
	const struct OPG_Solver_Volume*const opg_s_vol[2] = { (struct OPG_Solver_Volume*) face->neigh_info[0].volume,
	                                                      (struct OPG_Solver_Volume*) face->neigh_info[1].volume, };

	const struct const_Matrix_d*const lhs_r =
		constructor_mm_diag_const_Matrix_d_d(1.0,ops->cv0_vt_fc[side_index[1]],ops->wJ_fc,'L',false); // destructed

	const double scale = ( side_index[0] == side_index[1] ? -1.0 : 1.0 );
	const struct const_Matrix_d*const lhs =
		constructor_mm_const_Matrix_d('T','N',scale,ops->cv0_vt_fc[side_index[0]],lhs_r,'R'); // destructed
	destructor_const_Matrix_d(lhs_r);

	const int*const n_vr_eq = get_set_n_var_eq(NULL);
	const int n_vr    = n_vr_eq[0],
	          n_eq    = n_vr_eq[1];
	/** \warning It is possible that a change may be required when systems of equations are used. Currently, there is
	 *           a "default coupling" between the face terms between each equations and variables.
	 *
	 *  From a few of the DPG papers, it seems that an additional |n (dot) df/du| (note the absolution value) scaling
	 *  may need to be added. While this would introduce the coupling between the equations, it may also destroy the
	 *  symmetry for non scalar PDEs. Entropy variables to fix this or simply don't assume symmetric? THINK.
	 */
	 assert(n_vr == 1 && n_eq == 1); // Ensure that all is working properly when removed.

	for (int vr = 0; vr < n_vr; ++vr) {
	for (int eq = 0; eq < n_eq; ++eq) {
		set_petsc_Mat_row_col_opg(ssi,opg_s_vol[side_index[0]],eq,opg_s_vol[side_index[1]],vr);
		add_to_petsc_Mat(ssi,lhs);
	}}
}

const struct const_Matrix_d* constructor_lhs_f_1_b
	(const struct Numerical_Flux*const num_flux, const struct Solver_Face*const s_face)
{
	const struct const_Matrix_d*const lhs_l  = constructor_lhs_f_1_b_l(num_flux,s_face); // destructed
	const struct const_Matrix_d*const lhs_r  = constructor_lhs_f_1_b_r(s_face);          // destructed

	const struct const_Matrix_T*const lhs = constructor_mm_const_Matrix_T('N','N',1.0,lhs_l,lhs_r,'R'); // returned
	destructor_const_Matrix_T(lhs_l);
	destructor_const_Matrix_T(lhs_r);

	return lhs;
}

// Level 3 ********************************************************************************************************** //

static struct const_Matrix_d* constructor_lhs_f_1_b_l
	(const struct Numerical_Flux*const num_flux, const struct Solver_Face*const s_face)
{
	const int*const n_vr_eq = get_set_n_var_eq(NULL);
	const int n_vr = n_vr_eq[0],
	          n_eq = n_vr_eq[1];

	const struct Operator*const tw0_vt_fc = get_operator__tw0_vt_fc_T(0,s_face);
	const ptrdiff_t ext_0 = tw0_vt_fc->op_std->ext_0,
	                ext_1 = tw0_vt_fc->op_std->ext_1;

	struct Matrix_T*const tw0_nf = constructor_empty_Matrix_T('R',ext_0,ext_1);            // destructed
	struct Matrix_T*const lhs_l  = constructor_empty_Matrix_T('R',n_eq*ext_0,n_vr*ext_1); // returned
	set_to_value_Matrix_T(tw0_nf,0.0);

	const struct const_Multiarray_T*const dnnf_ds_Ma = num_flux->neigh_info[0].dnnf_ds;
	struct Vector_T dnnf_ds = { .ext_0 = dnnf_ds_Ma->extents[0], .owns_data = false, .data = NULL, };

	for (int vr = 0; vr < n_vr; ++vr) {
	for (int eq = 0; eq < n_eq; ++eq) {
		const ptrdiff_t ind =
			compute_index_sub_container(dnnf_ds_Ma->order,1,dnnf_ds_Ma->extents,(ptrdiff_t[]){eq,vr});
		dnnf_ds.data = (Type*)&dnnf_ds_Ma->data[ind];
		mm_diag_T('R',1.0,0.0,tw0_vt_fc->op_std,(struct const_Vector_T*)&dnnf_ds,tw0_nf,false);

		set_block_Matrix_T(lhs_l,eq*tw0_nf->ext_0,vr*tw0_nf->ext_1,
		                   (struct const_Matrix_T*)tw0_nf,0,0,tw0_nf->ext_0,tw0_nf->ext_1,'i');
	}}
	destructor_Matrix_T(tw0_nf);

	const struct const_Vector_R j_det_fc    = interpret_const_Multiarray_as_Vector_R(s_face->jacobian_det_fc);
	const struct const_Vector_R*const jr_fc = constructor_repeated_const_Vector_R(1.0,&j_det_fc,n_vr); // destructed

	scale_Matrix_by_Vector_T('R',1.0,lhs_l,jr_fc,false);
	destructor_const_Vector_R(jr_fc);

	return (struct const_Matrix_d*) lhs_l;
}

static struct const_Matrix_d* constructor_lhs_f_1_b_r (const struct Solver_Face*const s_face)
{
	const int*const n_vr_eq = get_set_n_var_eq(NULL);
	const int n_vr = n_vr_eq[0],
	          n_eq = n_vr_eq[1];

	const struct OPG_Solver_Face*const opg_s_face = (struct OPG_Solver_Face*) s_face;
	const struct Multiarray_Operator cv1_vt_fc = get_operator__cv1_vt_fc(0,opg_s_face);
	const ptrdiff_t ext_0 = cv1_vt_fc.data[0]->op_std->ext_0,
	                ext_1 = cv1_vt_fc.data[0]->op_std->ext_1;

	struct Matrix_T*const ds_dtc = constructor_empty_Matrix_T('R',ext_0,ext_1);           // destructed
	struct Matrix_T*const lhs_r  = constructor_empty_Matrix_T('R',n_vr*ext_0,n_eq*ext_1); // returned

EXIT_ADD_SUPPORT;
//	const struct const_Multiarray_d*const dfr_ds_Ma = flux_r->dfr_ds;
	const struct const_Multiarray_d*const dfr_ds_Ma = NULL;
	struct Vector_d dfr_ds = { .ext_0 = dfr_ds_Ma->extents[0], .owns_data = false, .data = NULL, };

	for (int vr = 0; vr < n_vr; ++vr) {
	for (int eq = 0; eq < n_eq; ++eq) {
		set_to_value_Matrix_d(ds_dtc,0.0);
		for (int dim = 0; dim < DIM; ++dim) {
			const ptrdiff_t ind =
				compute_index_sub_container(dfr_ds_Ma->order,1,dfr_ds_Ma->extents,(ptrdiff_t[]){eq,vr,dim});
			dfr_ds.data = (double*)&dfr_ds_Ma->data[ind];
			mm_diag_d('L',-1.0,1.0,cv1_vt_fc.data[dim]->op_std,(struct const_Vector_d*)&dfr_ds,ds_dtc,false);
		}
		// Note the swapping of vr and eq below for the transpose of df_ds.
		set_block_Matrix_d(lhs_r,vr*ext_0,eq*ext_1,
		                   (struct const_Matrix_d*)ds_dtc,0,0,ds_dtc->ext_0,ds_dtc->ext_1,'i');
	}}
	destructor_Matrix_T(ds_dtc);

	const struct const_Vector_R j_det_fc    = interpret_const_Multiarray_as_Vector_R(s_face->vol_jacobian_det_fc);
	const struct const_Vector_R*const jr_fc = constructor_repeated_const_Vector_R(1.0,&j_det_fc,n_vr); // destructed

	scale_Matrix_by_Vector_T('L',1.0,lhs_r,jr_fc,true);
	destructor_const_Vector_R(jr_fc);

	return (struct const_Matrix_d*) lhs_r;
}