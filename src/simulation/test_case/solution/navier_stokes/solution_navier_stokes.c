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

#include "solution_navier_stokes.h"

#include "boundary.h"
//#include "compute_error_navier_stokes.h"
#include "solution.h"
#include "flux_euler.h"
//#include "flux_navier_stokes.h"
#include "geometry.h"
#include "geometry_parametric.h"
#include "numerical_flux_euler.h"
//#include "numerical_flux_navier_stokes.h"
#include "simulation.h"
#include "test_case.h"

//#include "periodic_vortex/solution_periodic_vortex.h"

// Static function declarations ************************************************************************************* //

// Interface functions ********************************************************************************************** //

#include "def_templates_type_d.h"
#include "solution_navier_stokes_T.c"

// Static functions ************************************************************************************************* //
// Level 0 ********************************************************************************************************** //