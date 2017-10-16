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

#ifndef DPG__solution_supersonic_vortex_h__INCLUDED
#define DPG__solution_supersonic_vortex_h__INCLUDED
/** \file
 *  \brief Provides the interface to functions used for solution computation for the supersonic vortex test case.
 */

struct Simulation;
struct Solver_Volume;
struct Solver_Face;

/// \brief Function pointer to be used for compute_sol_coef_v for the supersonic vortex test case.
void compute_sol_coef_v_supersonic_vortex
	(const struct Simulation* sim, ///< Defined for \ref compute_sol_coef_v_fptr.
	 struct Solver_Volume* volume  ///< Defined for \ref compute_sol_coef_v_fptr.
	);

/// \brief Function pointer to be used for compute_sol_coef_f for the supersonic vortex test case.
void compute_sol_coef_f_supersonic_vortex
	(const struct Simulation* sim, ///< Defined for \ref compute_sol_coef_f_fptr.
	 struct Solver_Face* face      ///< Defined for \ref compute_sol_coef_f_fptr.
	);

#endif // DPG__solution_supersonic_vortex_h__INCLUDED