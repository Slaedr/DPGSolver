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
 *  \brief Provides the interface to templated functions used for blended geometry processing.
 */

struct Solver_Volume_T;
struct Simulation;

/** \brief Version of \ref constructor_xyz_fptr_T for the blended curved volume geometry.
 *  \return See brief. */
const struct const_Multiarray_R* constructor_xyz_blended_T
	(const char n_type,                      ///< See brief.
	 const struct const_Multiarray_R* xyz_i, ///< See brief.
	 const struct Solver_Volume_T* s_vol,    ///< See brief.
	 const struct Simulation* sim            ///< See brief.
	);
