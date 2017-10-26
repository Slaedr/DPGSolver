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

#include "solution_periodic_vortex.h"

#include <assert.h>
#include <math.h>
#include <string.h>

#include "macros.h"
#include "definitions_intrusive.h"
#include "definitions_math.h"
#include "definitions_tol.h"

#include "multiarray.h"

#include "element_solution.h"
#include "volume_solver.h"

#include "file_processing.h"
#include "multiarray_operator.h"
#include "operator.h"
#include "simulation.h"
#include "solution.h"
#include "solution_euler.h"
#include "test_case.h"

// Static function declarations ************************************************************************************* //

/// \brief Container for solution data relating to 'p'eriodic 'v'ortex.
struct Sol_Data__pv {
	// Read parameters
	double r_v,   ///< A parameter related to the radial decay of the vortex.
	       theta, ///< The angle at which the vortex propagates in the xy-plane.
	       V_inf, ///< The reference speed of the vortex in the theta-direction.
	       p_inf, ///< The reference pressure.
	       t_inf, ///< The reference temperature.
	       Rg;    ///< The gas constant.

	// Additional parameters
	double period_l, ///< The 'l'ength of the period.
	       rho_inf,  ///< The reference density.
	       u_inf,    ///< The speed of the vortex in the x-direction.
	       v_inf,    ///< The speed of the vortex in the y-direction.
	       con;      ///< A scaling constant.
};

/** \brief Return the statically \ref Sol_Data__pv container.
 *  \return See brief. */
static struct Sol_Data__pv get_sol_data
	(const struct Simulation* sim ///< \ref Simulation.
	);

/// \brief Read the required solution data into \ref Sol_Data__pv.
static void read_data_periodic_vortex
	(const char*const input_path,       ///< Defined in \ref fopen_input.
	 struct Sol_Data__pv*const sol_data ///< \ref Sol_Data__pv.
	);

/// \brief Set the remaining required solution data of \ref Sol_Data__pv based on the read values.
static void set_data_periodic_vortex
	(struct Sol_Data__pv*const sol_data ///< \ref Sol_Data__pv.
	);

/** \brief Return a \ref Multiarray_d\* container holding the solution values at the input coordinates.
 *  \return See brief. */
static struct Multiarray_d* constructor_sol_periodic_vortex
	(const struct Simulation* sim,         ///< Defined for \ref set_sol_v_periodic_vortex.
	 const struct const_Multiarray_d* xyz, ///< xyz coordinates at which to evaluate the solution.
	 const struct Sol_Data__pv* sol_data   ///< \ref Sol_Data__pv.
	);

/// \brief Version of \ref set_sol_f_periodic_vortex for coefficients.
static void set_sol_coef_f_periodic_vortex
	(const struct Simulation* sim, ///< Defined for \ref set_sol_f_periodic_vortex.
	 struct Solver_Face* face      ///< Defined for \ref set_sol_f_periodic_vortex.
	);

/// \brief Set the centre xy coordinates of the periodic vortex at the given time.
void set_xy_c
	(double* x_c,                         ///< The x-coordinate of the vortex centre.
	 double* y_c,                         ///< The y-coordinate of the vortex centre.
	 const struct Sol_Data__pv* sol_data, ///< \ref Sol_Data__pv.
	 const double time                    ///< \ref Test_Case::time.
	);

// Interface functions ********************************************************************************************** //

void set_sol_periodic_vortex (const struct Simulation* sim, struct Solution_Container sol_cont)
{
	const char ce_type   = sol_cont.ce_type,
	           cv_type   = sol_cont.cv_type,
	           node_kind = sol_cont.node_kind;

	assert(ce_type == 'v'); // Add support for faces if necessary.

	const struct Sol_Data__pv sol_data = get_sol_data(sim);

	const struct const_Multiarray_d* xyz = constructor_xyz_v(sim,sol_cont.volume,node_kind); // destructed
	struct Multiarray_d* sol = constructor_sol_periodic_vortex(sim,xyz,&sol_data);           // destructed
	destructor_const_Multiarray_d(xyz);

	assert((cv_type == 'c') || (cv_type == 'v'));
	if (cv_type == 'v') {
// possible function for this.
		sol_cont.sol->extents[0] = sol->extents[0];
		sol_cont.sol->extents[1] = sol->extents[1];
		sol_cont.sol->data = sol->data;

		sol->owns_data = false;
	} else if (cv_type == 'c') {
		assert(node_kind == 's');
		compute_coef_from_val_vs(sol_cont.volume,(struct const_Multiarray_d*)sol,sol_cont.sol);
	}

	destructor_Multiarray_d(sol);
}

///\todo make static
void set_sol_f_periodic_vortex
	(const struct Simulation* sim, struct Solver_Face* face, const char cv_type, const char node_kind)
{
	if (cv_type == 'c' && node_kind == 'c')
		set_sol_coef_f_periodic_vortex(sim,face);
	else
		EXIT_UNSUPPORTED;
}

// Static functions ************************************************************************************************* //
// Level 0 ********************************************************************************************************** //

static struct Sol_Data__pv get_sol_data (const struct Simulation* sim)
{
	static bool need_input = true;

	static struct Sol_Data__pv sol_data;
	if (need_input) {
		need_input = false;
		read_data_periodic_vortex(sim->input_path,&sol_data);
		set_data_periodic_vortex(&sol_data);
	}

	return sol_data;
}

static struct Multiarray_d* constructor_sol_periodic_vortex
	(const struct Simulation* sim, const struct const_Multiarray_d* xyz, const struct Sol_Data__pv* sol_data)
{
	assert(sim->d >= 2);

	// Set the coordinates of the vortex centre depending on the time.
	double x_c = 0.0,
	       y_c = 0.0;
	set_xy_c(&x_c,&y_c,sol_data,sim->test_case->time);

	// Compute the solution
	const ptrdiff_t n_vs = xyz->extents[0],
	                d    = xyz->extents[1];

	const double* x = get_col_const_Multiarray_d(0,xyz),
	            * y = get_col_const_Multiarray_d(1,xyz);

	const int n_var = sim->test_case->n_var;

	struct Multiarray_d* sol = constructor_empty_Multiarray_d('C',2,(ptrdiff_t[]){n_vs,n_var}); // returned

	double* rho = get_col_Multiarray_d(0,sol),
	      * u   = get_col_Multiarray_d(1,sol),
	      * v   = get_col_Multiarray_d(2,sol),
	      * p   = get_col_Multiarray_d(n_var-1,sol);
	for (int i = 0; i < n_vs; ++i) {
		const double rho_inf = sol_data->rho_inf,
		             u_inf   = sol_data->u_inf,
		             v_inf   = sol_data->v_inf,
		             p_inf   = sol_data->p_inf,
		             r_v     = sol_data->r_v,
		             con     = sol_data->con;

		const double r2 = (pow(x[i]-x_c,2.0)+pow(y[i]-y_c,2.0))/(r_v*r_v);

		rho[i] = rho_inf;
		u[i]   = u_inf - con*(y[i]-y_c)/(r_v*r_v)*exp(-0.5*r2);
		v[i]   = v_inf + con*(x[i]-x_c)/(r_v*r_v)*exp(-0.5*r2);
		p[i]   = p_inf - rho_inf*(con*con)/(2*r_v*r_v)*exp(-r2);
	}

	if (d == 3) {
		double* w = get_col_Multiarray_d(3,sol);
		for (int i = 0; i < n_vs; ++i)
			w[i] = 0.0;
	}
	convert_variables(sol,'p','c');

	return sol;
}

static void set_sol_coef_f_periodic_vortex (const struct Simulation* sim, struct Solver_Face* face)
{
UNUSED(face);
	switch (sim->method) {
	case METHOD_DG:
		set_sg_do_nothing(sim,(struct Solution_Container) {} );
		break;
	default:
		EXIT_ERROR("Unsupported: %d\n",sim->method);
		break;
	}
}

static void read_data_periodic_vortex (const char*const input_path, struct Sol_Data__pv*const sol_data)
{
	const int count_to_find = 6;

	FILE* input_file = fopen_input(input_path,'s'); // closed

	int count_found = 0;
	char line[STRLEN_MAX];
	while (fgets(line,sizeof(line),input_file)) {
		if (strstr(line,"r_v")) {
			++count_found;
			read_skip_d(line,&sol_data->r_v,1,false);
		} else if (strstr(line,"theta")) {
			++count_found;
			read_skip_d(line,&sol_data->theta,1,false);
		} else if (strstr(line,"V_inf")) {
			++count_found;
			read_skip_d(line,&sol_data->V_inf,1,false);
		} else if (strstr(line,"p_inf")) {
			++count_found;
			read_skip_d(line,&sol_data->p_inf,1,false);
		} else if (strstr(line,"t_inf")) {
			++count_found;
			read_skip_d(line,&sol_data->t_inf,1,false);
		} else if (strstr(line,"Rg")) {
			++count_found;
			read_skip_d(line,&sol_data->Rg,1,false);
		}
	}
	fclose(input_file);

	if (count_found != count_to_find)
		EXIT_ERROR("Did not find the required number of variables");
}

void set_xy_c (double* x_c, double* y_c, const struct Sol_Data__pv* sol_data, const double time)
{
	const double theta    = sol_data->theta;
	const double period_l = sol_data->period_l;
	const double V_inf    = sol_data->V_inf;

	const double period_frac = fmod(time*V_inf+0.5*period_l,period_l)/period_l;

	// As the solution is not actually specified by a periodic function, the solution should only be evaluated when it
	// is close to the centre of the domain.
	assert(fabs(period_frac-0.5) <= 0.05);

	*x_c = (period_frac-0.5)*period_l*cos(theta);
	*y_c = (period_frac-0.5)*period_l*sin(theta);
}

static void set_data_periodic_vortex (struct Sol_Data__pv*const sol_data)
{
	const double theta = sol_data->theta;
	assert(fmod(theta,PI/4.0) < EPS);

	if (fmod(sol_data->theta,PI/2.0) < EPS) {
		sol_data->period_l = 2.0;
	} else {
		sol_data->period_l = 2.0*sqrt(2.0);
	}

	sol_data->rho_inf = (sol_data->p_inf)/(sol_data->Rg*sol_data->t_inf);

	const double V_inf = sol_data->V_inf;
	sol_data->u_inf = V_inf*cos(theta);
	sol_data->v_inf = V_inf*sin(theta);

	if (fabs(sol_data->u_inf) < 1e-1*EPS)
		sol_data->u_inf = 1e-1*EPS;

	if (fabs(sol_data->v_inf) < 1e-1*EPS)
		sol_data->v_inf = 1e-1*EPS;

	sol_data->con = 0.1*V_inf;
}
