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
 *  \brief Provides the templated Euler flux functions.
 *  \todo Clean-up.
 */

#include <stddef.h>

#include "macros.h"
#include "definitions_core.h"
#include "definitions_test_case.h"

#include "multiarray.h"

#include "flux.h"

// Static function declarations ************************************************************************************* //

#define NEQ  NEQ_EULER  ///< Number of equations.
#define NVAR NVAR_EULER ///< Number of variables.

// Interface functions ********************************************************************************************** //

/** \brief Version of \ref compute_Flux_fptr computing the fluxes for the Euler equations.
 *
 *  Euler fluxes (eq, dim):
 *  \f[
 *  f =
 *  \begin{bmatrix}
 *  	\rho u & \rho v & \rho w \\
 *  	\rho u^2 + p & \rho uv & \rho uw \\
 *  	\rho vu & \rho v^2 + p & \rho vw \\
 *  	\rho wu & \rho wv & \rho w^2 + p \\
 *  	(E+p)u & (E+p)v & (E+p)w \\
 *  \end{bmatrix}
 *  \f]
 */
void compute_Flux_T_euler
	(const struct Flux_Input_T* flux_i, ///< See brief.
	 struct mutable_Flux_T* flux        ///< See brief.
	)
{
	const ptrdiff_t NnTotal = flux_i->s->extents[0];

	Type const *const W = flux_i->s->data;
	Type       *const F = flux->f->data;

	Type const *rho_ptr  = &W[NnTotal*0],
	           *rhou_ptr = &W[NnTotal*1],
	           *E_ptr    = &W[NnTotal*(DIM+1)];

	Type *F_ptr[DIM*NEQ];
	for (int eq = 0; eq < NEQ; eq++)  {
	for (int dim = 0; dim < DIM; dim++) {
		F_ptr[eq*DIM+dim] = &F[(eq*DIM+dim)*NnTotal];
	}}

	if (DIM == 3) {
		Type const *rhov_ptr = &W[NnTotal*2],
		           *rhow_ptr = &W[NnTotal*3];

		for (ptrdiff_t n = 0; n < NnTotal; n++) {
			Type const rho  = *rho_ptr++,
			           rhou = *rhou_ptr++,
			           rhov = *rhov_ptr++,
			           rhow = *rhow_ptr++,
			           E    = *E_ptr++,

			           u   = rhou/rho,
			           v   = rhov/rho,
			           w   = rhow/rho,

			           p = GM1*(E-0.5*rho*(u*u+v*v+w*w));

			ptrdiff_t IndF = 0;
			// eq 1
			*F_ptr[IndF++]++ += rhou;
			*F_ptr[IndF++]++ += rhov;
			*F_ptr[IndF++]++ += rhow;

			// eq 2
			*F_ptr[IndF++]++ += rhou*u + p;
			*F_ptr[IndF++]++ += rhou*v;
			*F_ptr[IndF++]++ += rhou*w;

			// eq 3
			*F_ptr[IndF++]++ += rhov*u;
			*F_ptr[IndF++]++ += rhov*v + p;
			*F_ptr[IndF++]++ += rhov*w;

			// eq 4
			*F_ptr[IndF++]++ += rhow*u;
			*F_ptr[IndF++]++ += rhow*v;
			*F_ptr[IndF++]++ += rhow*w + p;

			// eq 5
			*F_ptr[IndF++]++ += (E+p)*u;
			*F_ptr[IndF++]++ += (E+p)*v;
			*F_ptr[IndF++]++ += (E+p)*w;
		}
	} else if (DIM == 2) {
		Type const *rhov_ptr = &W[NnTotal*2];

		for (ptrdiff_t n = 0; n < NnTotal; n++) {
			Type const rho  = *rho_ptr++,
			           rhou = *rhou_ptr++,
			           rhov = *rhov_ptr++,
			           E    = *E_ptr++,

			           u   = rhou/rho,
			           v   = rhov/rho,

			           p = GM1*(E-0.5*rho*(u*u+v*v));

			ptrdiff_t IndF = 0;
			// eq 1
			*F_ptr[IndF++]++ += rhou;
			*F_ptr[IndF++]++ += rhov;

			// eq 2
			*F_ptr[IndF++]++ += rhou*u + p;
			*F_ptr[IndF++]++ += rhou*v;

			// eq 3
			*F_ptr[IndF++]++ += rhov*u;
			*F_ptr[IndF++]++ += rhov*v + p;

			// eq 4
			*F_ptr[IndF++]++ += (E+p)*u;
			*F_ptr[IndF++]++ += (E+p)*v;
		}
	} else if (DIM == 1) {
		for (ptrdiff_t n = 0; n < NnTotal; n++) {
			Type const rho  = *rho_ptr++,
			           rhou = *rhou_ptr++,
			           E    = *E_ptr++,

			           u   = rhou/rho,

			           p = GM1*(E-0.5*rho*(u*u));

			ptrdiff_t IndF = 0;
			// eq 1
			*F_ptr[IndF++]++ += rhou;

			// eq 2
			*F_ptr[IndF++]++ += rhou*u + p;

			// eq 3
			*F_ptr[IndF++]++ += (E+p)*u;
		}
	}
}

/** \brief Version of \ref compute_Flux_fptr computing the fluxes and Jacobians for the Euler equations.
 *
 *  Euler flux Jacobians (eq, var, dim):
 *  \f[
 *  \frac{df}{ds} =
 *  \begin{bmatrix}
 *  \begin{bmatrix}
 *  	 0                          &  1               &  0               &  0               & 0        \\
 *  	-u^2+\frac{\gamma-1}{2}V^2  & -(\gamma-3)u     & -(\gamma-1)v     & -(\gamma-1)w     & \gamma-1 \\
 *  	-uv                         &  v               &  u               &  0               & 0        \\
 *  	-uw                         &  w               &  0               &  u               & 0        \\
 *  	 u(\frac{\gamma-1}{2}V^2-H) &  H-(\gamma-1)u^2 & -(\gamma-1)uv    & -(\gamma-1)uw    & \gamma u \\
 *  \end{bmatrix},
 *  \begin{bmatrix}
 *  	 0                          &  0               &  1               &  0               & 0        \\
 *  	-uv                         &  v               &  u               &  0               & 0        \\
 *  	-v^2+\frac{\gamma-1}{2}V^2  & -(\gamma-1)u     & -(\gamma-3)v     & -(\gamma-1)w     & \gamma-1 \\
 *  	-vw                         &  0               &  w               &  v               & 0        \\
 *  	 v(\frac{\gamma-1}{2}V^2-H) & -(\gamma-1)uv    &  H-(\gamma-1)v^2 & -(\gamma-1)vw    & \gamma v \\
 *  \end{bmatrix},
 *  \begin{bmatrix}
 *  	 0                          &  0               &  0               &  1               & 0        \\
 *  	-uw                         &  w               &  0               &  u               & 0        \\
 *  	-vw                         &  0               &  w               &  v               & 0        \\
 *  	-w^2+\frac{\gamma-1}{2}V^2  & -(\gamma-1)u     & -(\gamma-1)v     & -(\gamma-3)w     & \gamma-1 \\
 *  	 w(\frac{\gamma-1}{2}V^2-H) & -(\gamma-1)uw    & -(\gamma-1)vw    &  H-(\gamma-1)w^2 & \gamma w \\
 *  \end{bmatrix},
 *  \end{bmatrix}
 *  \f]
 */
void compute_Flux_T_euler_jacobian
	(const struct Flux_Input_T* flux_i, ///< See brief.
	 struct mutable_Flux_T* flux        ///< See brief.
	)
{
	const ptrdiff_t NnTotal = flux_i->s->extents[0];

	Type const *const W    = flux_i->s->data;
	Type       *const F    = flux->f->data;
	Type       *const dFdW = flux->df_ds->data;

	// Standard datatypes
	int i, n, eq, var, dim, iMax, InddFdW;
	Type       rho, u, v, w, u2, uv, uw, v2, vw, w2, V2, E, p, H, alpha, beta, *dFdW_ptr[DMAX*NEQ*NEQ];
	const Type *rho_ptr, *rhou_ptr, *rhov_ptr, *rhow_ptr, *E_ptr;

	rho_ptr  = &W[NnTotal*0];
	rhou_ptr = &W[NnTotal*1];
	E_ptr    = &W[NnTotal*(DIM+1)];

	// Store pointers to the arrays that the data will be written into.
	Type *F_ptr[DMAX*NEQ];
	if (F != NULL) {
		for (eq = 0; eq < NEQ; eq++) {
		for (dim = 0; dim < DIM; dim++) {
			F_ptr[eq*DMAX+dim] = &F[(eq*DIM+dim)*NnTotal];
		}}
	}

	// Note: dFdW is ordered by [node, dim, eq, var] but is set by [node,dim,var,eq] below.
	for (var = 0; var < NVAR; var++) {
	for (eq = 0; eq < NEQ; eq++) {
	for (dim = 0; dim < DIM; dim++) {
		const int ind_dm = dim+DMAX*(var+NVAR*(eq)),
		          ind_d  = dim+DIM*(eq+NEQ*(var));
		dFdW_ptr[ind_dm] = &dFdW[NnTotal*ind_d];
	}}}

	if (DIM == 3) {
		rhov_ptr = &W[NnTotal*2];
		rhow_ptr = &W[NnTotal*3];

		for (n = 0; n < NnTotal; n++) {
			rho  = *rho_ptr;
			Type const rhou = *rhou_ptr,
			           rhov = *rhov_ptr,
			           rhow = *rhow_ptr;
			u   = rhou/rho;
			v   = rhov/rho;
			w   = rhow/rho;
			E   = *E_ptr;


			u2 = u*u;
			uv = u*v;
			uw = u*w;
			v2 = v*v;
			vw = v*w;
			w2 = w*w;

			V2 = u2+v2+w2;
			p  = GM1*(E-0.5*rho*V2);
			H  = (E+p)/rho;

			alpha = 0.5*GM1*V2;
			beta  = alpha-H;

			if (F != NULL) {
				ptrdiff_t IndF = 0;
				// eq 1
				*F_ptr[IndF++] += rhou;
				*F_ptr[IndF++] += rhov;
				*F_ptr[IndF++] += rhow;

				// eq 2
				*F_ptr[IndF++] += rhou*u + p;
				*F_ptr[IndF++] += rhou*v;
				*F_ptr[IndF++] += rhou*w;

				// eq 3
				*F_ptr[IndF++] += rhov*u;
				*F_ptr[IndF++] += rhov*v + p;
				*F_ptr[IndF++] += rhov*w;

				// eq 4
				*F_ptr[IndF++] += rhow*u;
				*F_ptr[IndF++] += rhow*v;
				*F_ptr[IndF++] += rhow*w + p;

				// eq 5
				*F_ptr[IndF++] += (E+p)*u;
				*F_ptr[IndF++] += (E+p)*v;
				*F_ptr[IndF++] += (E+p)*w;

				for (i = 0, iMax = NEQ*DMAX; i < iMax; i++)
					F_ptr[i]++;
			}

			InddFdW = 0;
			// *** eq 1 ***
			// var 1
			*dFdW_ptr[InddFdW++] +=  0.0;
			*dFdW_ptr[InddFdW++] +=  0.0;
			*dFdW_ptr[InddFdW++] +=  0.0;

			// var 2
			*dFdW_ptr[InddFdW++] +=  1.0;
			*dFdW_ptr[InddFdW++] +=  0.0;
			*dFdW_ptr[InddFdW++] +=  0.0;

			// var 3
			*dFdW_ptr[InddFdW++] +=  0.0;
			*dFdW_ptr[InddFdW++] +=  1.0;
			*dFdW_ptr[InddFdW++] +=  0.0;

			// var 4
			*dFdW_ptr[InddFdW++] +=  0.0;
			*dFdW_ptr[InddFdW++] +=  0.0;
			*dFdW_ptr[InddFdW++] +=  1.0;

			// var 5
			*dFdW_ptr[InddFdW++] +=  0.0;
			*dFdW_ptr[InddFdW++] +=  0.0;
			*dFdW_ptr[InddFdW++] +=  0.0;

			// *** eq 2 ***
			// var 1
			*dFdW_ptr[InddFdW++] += -u2+alpha;
			*dFdW_ptr[InddFdW++] += -uv;
			*dFdW_ptr[InddFdW++] += -uw;

			// var 2
			*dFdW_ptr[InddFdW++] += -GM3*u;
			*dFdW_ptr[InddFdW++] +=  v;
			*dFdW_ptr[InddFdW++] +=  w;

			// var 3
			*dFdW_ptr[InddFdW++] += -GM1*v;
			*dFdW_ptr[InddFdW++] +=  u;
			*dFdW_ptr[InddFdW++] +=  0.0;

			// var 4
			*dFdW_ptr[InddFdW++] += -GM1*w;
			*dFdW_ptr[InddFdW++] +=  0.0;
			*dFdW_ptr[InddFdW++] +=  u;

			// var 5
			*dFdW_ptr[InddFdW++] +=  GM1;
			*dFdW_ptr[InddFdW++] +=  0.0;
			*dFdW_ptr[InddFdW++] +=  0.0;

			// *** eq 3 ***
			// var 1
			*dFdW_ptr[InddFdW++] += -uv;
			*dFdW_ptr[InddFdW++] += -v2+alpha;
			*dFdW_ptr[InddFdW++] += -vw;

			// var 2
			*dFdW_ptr[InddFdW++] +=  v;
			*dFdW_ptr[InddFdW++] += -GM1*u;
			*dFdW_ptr[InddFdW++] +=  0.0;

			// var 3
			*dFdW_ptr[InddFdW++] +=  u;
			*dFdW_ptr[InddFdW++] += -GM3*v;
			*dFdW_ptr[InddFdW++] +=  w;

			// var 4
			*dFdW_ptr[InddFdW++] +=  0.0;
			*dFdW_ptr[InddFdW++] += -GM1*w;
			*dFdW_ptr[InddFdW++] +=  v;

			// var 5
			*dFdW_ptr[InddFdW++] +=  0.0;
			*dFdW_ptr[InddFdW++] +=  GM1;
			*dFdW_ptr[InddFdW++] +=  0.0;

			// *** eq 4 ***
			// var 1
			*dFdW_ptr[InddFdW++] += -uw;
			*dFdW_ptr[InddFdW++] += -vw;
			*dFdW_ptr[InddFdW++] += -w2+alpha;

			// var 2
			*dFdW_ptr[InddFdW++] +=  w;
			*dFdW_ptr[InddFdW++] +=  0.0;
			*dFdW_ptr[InddFdW++] += -GM1*u;

			// var 3
			*dFdW_ptr[InddFdW++] +=  0.0;
			*dFdW_ptr[InddFdW++] +=  w;
			*dFdW_ptr[InddFdW++] += -GM1*v;

			// var 4
			*dFdW_ptr[InddFdW++] +=  u;
			*dFdW_ptr[InddFdW++] +=  v;
			*dFdW_ptr[InddFdW++] += -GM3*w;

			// var 5
			*dFdW_ptr[InddFdW++] +=  0.0;
			*dFdW_ptr[InddFdW++] +=  0.0;
			*dFdW_ptr[InddFdW++] +=  GM1;

			// *** eq 5 ***
			// var 1
			*dFdW_ptr[InddFdW++] +=  u*beta;
			*dFdW_ptr[InddFdW++] +=  v*beta;
			*dFdW_ptr[InddFdW++] +=  w*beta;

			// var 2
			*dFdW_ptr[InddFdW++] +=  H-GM1*u2;
			*dFdW_ptr[InddFdW++] += -GM1*uv;
			*dFdW_ptr[InddFdW++] += -GM1*uw;

			// var 3
			*dFdW_ptr[InddFdW++] += -GM1*uv;
			*dFdW_ptr[InddFdW++] +=  H-GM1*v2;
			*dFdW_ptr[InddFdW++] += -GM1*vw;

			// var 4
			*dFdW_ptr[InddFdW++] += -GM1*uw;
			*dFdW_ptr[InddFdW++] += -GM1*vw;
			*dFdW_ptr[InddFdW++] +=  H-GM1*w2;

			// var 5
			*dFdW_ptr[InddFdW++] +=  GAMMA*u;
			*dFdW_ptr[InddFdW++] +=  GAMMA*v;
			*dFdW_ptr[InddFdW++] +=  GAMMA*w;

			rho_ptr++; rhou_ptr++; rhov_ptr++; rhow_ptr++; E_ptr++;
			for (i = 0, iMax = NEQ*NVAR*DMAX; i < iMax; i++)
				dFdW_ptr[i]++;
		}
	} else if (DIM == 2) {

		rhov_ptr = &W[NnTotal*2];
		for (n = 0; n < NnTotal; n++) {

			rho = *rho_ptr;
			Type const rhou = *rhou_ptr,
			           rhov = *rhov_ptr;
			u   = rhou/rho;
			v   = rhov/rho;
			E   = *E_ptr;

			u2 = u*u;
			uv = u*v;
			v2 = v*v;

			V2 = u2+v2;
			p  = GM1*(E-0.5*rho*V2);
			H  = (E+p)/rho;

			alpha = 0.5*GM1*V2;
			beta  = alpha-H;

			// F = Null in the implicit solver
			if (F != NULL) {
				ptrdiff_t IndF = 0;
				// eq 1
				*F_ptr[IndF++] += rhou;
				*F_ptr[IndF++] += rhov;
				IndF += 1;

				// eq 2
				*F_ptr[IndF++] += rhou*u + p;
				*F_ptr[IndF++] += rhou*v;
				IndF += 1;

				// eq 3
				*F_ptr[IndF++] += rhov*u;
				*F_ptr[IndF++] += rhov*v + p;
				IndF += 1;

				// eq 4
				*F_ptr[IndF++] += (E+p)*u;
				*F_ptr[IndF++] += (E+p)*v;

				for (i = 0, iMax = NEQ*DMAX; i < iMax; i++)
					F_ptr[i]++;
			}

			InddFdW = 0;
			// *** eq 1 ***
			// var 1
			*dFdW_ptr[InddFdW++] +=  0.0;
			*dFdW_ptr[InddFdW++] +=  0.0;
			InddFdW += 1;

			// var 2
			*dFdW_ptr[InddFdW++] +=  1.0;
			*dFdW_ptr[InddFdW++] +=  0.0;
			InddFdW += 1;

			// var 3
			*dFdW_ptr[InddFdW++] +=  0.0;
			*dFdW_ptr[InddFdW++] +=  1.0;
			InddFdW += 1;

			// var 4
			*dFdW_ptr[InddFdW++] +=  0.0;
			*dFdW_ptr[InddFdW++] +=  0.0;
			InddFdW += 1;

			// *** eq 2 ***
			// var 1
			*dFdW_ptr[InddFdW++] += -u2+alpha;
			*dFdW_ptr[InddFdW++] += -uv;
			InddFdW += 1;

			// var 2
			*dFdW_ptr[InddFdW++] += -GM3*u;
			*dFdW_ptr[InddFdW++] +=  v;
			InddFdW += 1;

			// var 3
			*dFdW_ptr[InddFdW++] += -GM1*v;
			*dFdW_ptr[InddFdW++] +=  u;
			InddFdW += 1;

			// var 4
			*dFdW_ptr[InddFdW++] +=  GM1;
			*dFdW_ptr[InddFdW++] +=  0.0;
			InddFdW += 1;

			// *** eq 3 ***
			// var 1
			*dFdW_ptr[InddFdW++] += -uv;
			*dFdW_ptr[InddFdW++] += -v2+alpha;
			InddFdW += 1;

			// var 2
			*dFdW_ptr[InddFdW++] +=  v;
			*dFdW_ptr[InddFdW++] += -GM1*u;
			InddFdW += 1;

			// var 3
			*dFdW_ptr[InddFdW++] +=  u;
			*dFdW_ptr[InddFdW++] += -GM3*v;
			InddFdW += 1;

			// var 4
			*dFdW_ptr[InddFdW++] +=  0.0;
			*dFdW_ptr[InddFdW++] +=  GM1;
			InddFdW += 1;

			// *** eq 4 ***
			// var 1
			*dFdW_ptr[InddFdW++] +=  u*beta;
			*dFdW_ptr[InddFdW++] +=  v*beta;
			InddFdW += 1;

			// var 2
			*dFdW_ptr[InddFdW++] +=  H-GM1*u2;
			*dFdW_ptr[InddFdW++] += -GM1*uv;
			InddFdW += 1;

			// var 3
			*dFdW_ptr[InddFdW++] += -GM1*uv;
			*dFdW_ptr[InddFdW++] +=  H-GM1*v2;
			InddFdW += 1;

			// var 4
			*dFdW_ptr[InddFdW++] +=  GAMMA*u;
			*dFdW_ptr[InddFdW++] +=  GAMMA*v;

			rho_ptr++; rhou_ptr++; rhov_ptr++; E_ptr++;

			for (i = 0, iMax = NEQ*NVAR*DMAX; i < iMax; i++)
				dFdW_ptr[i]++;
		}
	} else if (DIM == 1) {
		for (n = 0; n < NnTotal; n++) {
			rho = *rho_ptr;
			Type const rhou = *rhou_ptr;
			u   = rhou/rho;
			E   = *E_ptr;

			u2 = u*u;

			V2 = u2;
			p  = GM1*(E-0.5*rho*V2);
			H  = (E+p)/rho;

			alpha = 0.5*GM1*V2;
			beta  = alpha-H;

			if (F != NULL) {
				ptrdiff_t IndF = 0;
				// eq 1
				*F_ptr[IndF++] += rhou;
				IndF += 2;

				// eq 2
				*F_ptr[IndF++] += rhou*u + p;
				IndF += 2;

				// eq 3
				*F_ptr[IndF++] += (E+p)*u;
				IndF += 2;

				for (i = 0, iMax = NEQ*DMAX; i < iMax; i++)
					F_ptr[i]++;
			}

			InddFdW = 0;
			// *** eq 1 ***
			// var 1
			*dFdW_ptr[InddFdW++] +=  0.0;
			InddFdW += 2;

			// var 2
			*dFdW_ptr[InddFdW++] +=  1.0;
			InddFdW += 2;

			// var 3
			*dFdW_ptr[InddFdW++] +=  0.0;
			InddFdW += 2;

			// *** eq 2 ***
			// var 1
			*dFdW_ptr[InddFdW++] += -u2+alpha;
			InddFdW += 2;

			// var 2
			*dFdW_ptr[InddFdW++] += -GM3*u;
			InddFdW += 2;

			// var 3
			*dFdW_ptr[InddFdW++] +=  GM1;
			InddFdW += 2;

			// *** eq 3 ***
			// var 1
			*dFdW_ptr[InddFdW++] +=  u*beta;
			InddFdW += 2;

			// var 2
			*dFdW_ptr[InddFdW++] +=  H-GM1*u2;
			InddFdW += 2;

			// var 3
			*dFdW_ptr[InddFdW++] +=  GAMMA*u;

			rho_ptr++; rhou_ptr++; E_ptr++;
			for (i = 0, iMax = NEQ*NVAR*DMAX; i < iMax; i++)
				dFdW_ptr[i]++;
		}
	}
}

// Static functions ************************************************************************************************* //
// Level 0 ********************************************************************************************************** //
