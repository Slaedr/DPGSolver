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

#ifndef DPG__solve_h__INCLUDED
#define DPG__solve_h__INCLUDED
/** \file
 *  \brief Provides the interface to functions used to solve for the solution.
 */

#include <stddef.h>
#include "petscmat.h"
#include "petscvec.h"

struct Simulation;

/// \brief Container holding members relating to memory storage for the implicit solver.
struct Solver_Storage_Implicit {
	Mat A; ///< Petsc Mat holding the LHS entries.
	Vec b; ///< Petsc Vec holding the negative of the RHS entries.

	int row, ///< Index of the first row in which data is to be added.
	    col; ///< Index of the first col in which data is to be added.
};

// Interface functions ********************************************************************************************** //

/// \brief Solve for the solution.
void solve_for_solution
	(struct Simulation* sim ///< \ref Simulation.
	);

/** \brief Compute the volume and face rhs terms for the the given method.
 *  \return The maximum absolute value of the rhs.
 *
 *  The rhs includes all terms of the discretization except for the time-varying term but **is scaled by the inverse
 *  mass matrix** (i.e. \f$ M_v \frac{\partial}{\partial t} \text{sol_coef} = \text{rhs} \rightarrow \text{rhs} =
 *  M_v^{-1}\text{rhs} \f$).
 */
double compute_rhs
	(const struct Simulation* sim ///< \ref Simulation.
	);

/** \brief Compute the volume and face rhs and lhs terms for the the given method.
 *  \return The maximum absolute value of the rhs.
 *
 *  The rhs includes all terms of the discretization. Unlike for \ref compute_rhs, rhs terms **are not scaled by the
 *  inverse mass matrix**.
 */
double compute_rlhs
	(const struct Simulation* sim,             ///< \ref Simulation.
	 struct Solver_Storage_Implicit* s_store_i ///< \ref Solver_Storage_Implicit.
	);

/** \brief Compute the number of 'd'egrees 'o'f 'f'reedom.
 *  \return See brief. */
ptrdiff_t compute_dof
	(const struct Simulation* sim ///< \ref Simulation.
	);

#endif // DPG__solve_h__INCLUDED
