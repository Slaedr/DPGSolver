// Copyright 2017 Philip Zwanenburg
// MIT License (https://github.com/PhilipZwanenburg/DPGSolver/blob/master/LICENSE)

#include "test_integration_Poisson.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "petscmat.h"

#include "Parameters.h"
#include "Macros.h"
#include "S_DB.h"
#include "S_VOLUME.h"
#include "S_FACE.h"
#include "Test.h"

#include "test_code_integration.h"
#include "test_support.h"
#include "test_integration_linearization.h"
#include "test_integration_Euler.h"
#include "compute_errors.h"
#include "array_norm.h"
#include "solver_Poisson.h"
#include "finalize_LHS.h"
#include "adaptation.h"
#include "initialize_test_case.h"
#include "output_to_paraview.h"
#include "array_free.h"

/*
 *	Purpose:
 *		Test various aspects of the Poisson solver implementation:
 *			1) Linearization
 *			2) Optimal convergence orders
 *
 *	Comments:
 *
 *		It was very difficult to find a case where it was clear that blending of a curved boundary was leading to a loss
 *		of optimal convergence, despite the potentially unbounded mapping derivatives with h-refinement required for the
 *		optimal error estimate in Ciarlet(1972) (Theorem 5). As noted in Scott(1973) (p. 54), the mapping function and
 *		all of its derivatives are bounded IN TERMS OF THE BOUNDARY CURVATUVE AND ITS DERIVATIVES, which motivated the
 *		implementation of cases with geometry possessing high element curvatuve on coarse meshes. For these cases,
 *		optimal convergence is lost until the mesh has been refined "enough" (such that the element curvature is small)
 *		at which point it is recovered. Perhaps even more significant, the L2 error of the projection of the exact
 *		solution sometimes increased with mesh refinement on coarse meshes (run with Compute_L2proj = 1), a trend which
 *		was not observed for the computed solution. There was no modification found which could serve to fix this issue
 *		(such as the use of alternate blending functions or generalizations of the high-order blending used to fix the
 *		NIELSON blending as proposed by Lenoir(1986)). The mathematical and numerical support for this observation can
 *		be found in Zwanenburg(2017).
 *
 *		As a results of the conclusions of the study performed in Zwanenburg(2017) (and based on my current
 *		understanding), it seems that any of the optimal blending functions should give analogous results (SCOTT,
 *		SZABO_BABUSKA, LENOIR) and that the NORMAL surface parametrization is best (certainly in the 2D case). For the
 *		extension to 3D blending, the approach to be taken would be to extend the SB blending to PYR elements (using a
 *		combination of GH and SB for the LINE and TRI parts of the WEDGE element). The surface parametrization for the
 *		non-edge geometry nodes could be use the NORMAL parametrization. An investigation into the optimal EDGE
 *		parametrization is still required.
 *
 *		It may also be noted that, despite converging at the same rate, the L2error of the computed solution is
 *		significantly higher than that of the L2 projected exact solution for certain polynomial orders. Comparison with
 *		results obtained based on the DPG solver may be interesting here. (ToBeModified)
 *
 *
 *	*** IMPORTANT ***   Convergence Order Testing   *** IMPORTANT ***
 *
 *		It was found that optimal convergence was not possible to obtain using a series of uniformly refined TET meshes
 *		based on the "refine by splitting" algorithm in gmsh. However, optimal orders were recovered when a series of
 *		unstructured meshes consisting of TETs of decreasing volume was used.
 *
 *		Gmsh(2.14)'s "refine by splitting" algorithm splits each TET into 8 TETs, in a manner identical to the TET8
 *		h-refinement algorithm implemented in the code. Assuming an initially regular TET, with all edges having length
 *		= 2.0, is refined in this manner, the result will be 4 regular TETs with edge length = 1.0 and 4 other TETs with
 *		5 edges having length = 1.0 and 1 edge having length = sqrt(2.0). Taking a measure of the regularity of the mesh
 *		to be the ratio of the spheres enclosing and enclosed by the TETs, the regularity bound is violated through this
 *		refinement process if the splitting direction of the internal octohedron is taken at random, but not if taken
 *		consistently along the same axis (See Lenoir(1986) for the regularity requirement). Hence an alternative method
 *		of generating the refined mesh sequence must be employed (as compared to gmsh) to achieve the optimal
 *		convergence:
 *
 *			1) A sequence of unstructured (non-nested) meshes (gmsh) with diminishing volume gave optimal orders.
 *			2) TET8 refinement along a consistent axis (this code) gave optimal orders.
 *			3) TET6 and TET12 refinement algorithms both gave sub-optimal orders to date.
 *				- Continued investigation may be made after a mesh quality improvement algorithm is implemented (e.g.
 *				  Peiro(2013)-Defining_Quality_Measures_for_Validation_and_Generation_of_High-Order_Tetrahedral_Meshes).
 *				  ToBeModified.
 *
 *	*** IMPORTANT ***   Convergence Order Testing   *** IMPORTANT ***
 *
 *		When testing with FLUX_IP, the iterative solver seems much more likely to fail based on several tests,
 *		regardless of the value selected for tau. The tolerance on the symmetry test might also need to be increased to
 *		show a passing result for this flux.
 *
 *	Notation:
 *
 *	References:
 *		Ciarlet(1972)-Interpolation_Theory_Over_Curved_Elements,_with_Applications_to_Finite_Element_Methods
 *		Scott(1973)-Finite_Element_Techniques_for_Curved_Boundaries
 *		Lenoir(1986)-Optimal_Isoparametric_Finite_Elements_and_Error_Estimates_for_Domains_Involving_Curved_Boundaries
 *		Zwanenburg(2017)-A_Necessary_High-Order_Meshing_Constraint_when_using_Polynomial_Blended_Geometry_Elements_for_
 *		                 Curved_Boundary_Representation
 */

struct S_linearization {
	Mat A, A_cs, A_csc;
	Vec b, b_cs, b_csc, x, x_cs, x_csc;
};

struct S_convorder {
	bool         PrintEnabled, Compute_L2proj, AdaptiveRefine, TestTRI;
	unsigned int PMin, PMax, MLMin, MLMax, Adapt, PG_add, IntOrder_add, IntOrder_mult;
	char         **argvNew, *PrintName;
};

char *get_fNameOut(char *output_type)
{
	char string[STRLEN_MIN], *fNameOut;

	fNameOut = malloc(STRLEN_MAX * sizeof *fNameOut);

	strcpy(fNameOut,output_type);
	sprintf(string,"%dD_",DB.d);   strcat(fNameOut,string);
								   strcat(fNameOut,DB.MeshType);
	if (DB.Adapt == ADAPT_0) {
		sprintf(string,"_ML%d",DB.ML); strcat(fNameOut,string);
		sprintf(string,"P%d_",DB.PGlobal); strcat(fNameOut,string);
	} else {
		sprintf(string,"_ML%d",TestDB.ML); strcat(fNameOut,string);
		sprintf(string,"P%d_",TestDB.PGlobal); strcat(fNameOut,string);
	}

	return fNameOut;
}

static void test_linearization(int nargc, char **argvNew, const unsigned int Nref, const unsigned int update_argv,
                               const char *TestName, struct S_linearization *data)
{
	unsigned int pass;

	PetscBool Symmetric;

	Mat A, A_cs, A_csc;
	Vec b, b_cs, b_csc, x, x_cs, x_csc;

	A = data->A; A_cs = data->A_cs; A_csc = data->A_csc;
	b = data->b; b_cs = data->b_cs; b_csc = data->b_csc;
	x = data->x; x_cs = data->x_cs; x_csc = data->x_csc;

	code_startup(nargc,argvNew,Nref,update_argv);

	implicit_info_Poisson();

	finalize_LHS(&A,&b,&x,0);
	compute_A_cs(&A_cs,&b_cs,&x_cs,0);
	compute_A_cs_complete(&A_csc,&b_csc,&x_csc);

//	MatView(A,PETSC_VIEWER_STDOUT_SELF);
//	MatView(A_cs,PETSC_VIEWER_STDOUT_SELF);

	MatIsSymmetric(A,1e3*EPS,&Symmetric);
//	MatIsSymmetric(A_cs,1e5*EPS,&Symmetric);

	pass = 0;
	if (PetscMatAIJ_norm_diff_d(DB.dof,A_cs,A,"Inf")     < 1e2*EPS &&
	    PetscMatAIJ_norm_diff_d(DB.dof,A_cs,A_csc,"Inf") < 1e2*EPS &&
	    Symmetric)
		pass = 1, TestDB.Npass++;
	else
		printf("%e %e %d\n",PetscMatAIJ_norm_diff_d(DB.dof,A_cs,A,"Inf"),
		                    PetscMatAIJ_norm_diff_d(DB.dof,A_cs,A_csc,"Inf"),Symmetric);

	printf("%s",TestName);
	test_print(pass);

	finalize_ksp(&A,&b,&x,2);
	finalize_ksp(&A_cs,&b_cs,&x_cs,2);
	finalize_ksp(&A_csc,&b_csc,&x_csc,2);
	code_cleanup();
}

static void set_test_convorder_data(struct S_convorder *data, const char *TestName)
{
	/*
	 *	Comments:
	 *		As nodes having integration strength of order 2*P form a basis for the polynomial space of order P for TP
	 *		elements and SI elements (P <= 2), non-trivial L2 projection error requires:
	 *			TP         : InOrder_add > 1
	 *			SI (P <= 2): InOrder_add > 0
	 */

	// default values
	data->PrintEnabled   = 0;
	data->Compute_L2proj = 0;
	data->AdaptiveRefine = 0;
	data->Adapt = ADAPT_HP;

	data->PMin  = 1;
	data->PMax  = 3;
	data->MLMin = 0;
	data->MLMax = 4;

	data->PG_add        = 0;
	data->IntOrder_add  = 2;
	data->IntOrder_mult = 2;


	if (strstr(TestName,"n-Ellipsoid_HollowSection")) {
		if (strstr(TestName,"TRI")) {
			strcpy(data->argvNew[1],"test/Poisson/Test_Poisson_n-Ellipsoid_HollowSection_CurvedTRI");
		} else if (strstr(TestName,"QUAD")) {
			strcpy(data->argvNew[1],"test/Poisson/Test_Poisson_n-Ellipsoid_HollowSection_CurvedQUAD");
		} else {
			EXIT_UNSUPPORTED;
		}
	} else {
		EXIT_UNSUPPORTED;
	}
}

void set_PrintName_ConvOrders(char *PrintName, bool *TestTRI)
{
	if (!(*TestTRI)) {
		*TestTRI = 1;
		strcpy(PrintName,"Convergence Orders (");
	} else {
		strcpy(PrintName,"                   (");
	}

	strcat(PrintName,DB.PDE);          strcat(PrintName,", ");
	if (!strstr(DB.PDESpecifier,"NONE")) {
		strcat(PrintName,DB.PDESpecifier); strcat(PrintName,", ");
	}
	strcat(PrintName,DB.MeshType);     strcat(PrintName,") : ");
}

static void test_convorder(int nargc, char **argvNew, const char *TestName, struct S_convorder *data)
{
	unsigned int pass = 0;

	bool         PrintEnabled, Compute_L2proj, AdaptiveRefine;
	unsigned int Adapt, PMin, PMax, MLMin, MLMax;
	double       *mesh_quality;

	set_test_convorder_data(data,TestName);

	PrintEnabled   = data->PrintEnabled;
	Compute_L2proj = data->Compute_L2proj;
	AdaptiveRefine = data->AdaptiveRefine;
	Adapt          = data->Adapt;

	PMin  = data->PMin;  PMax  = data->PMax;
	MLMin = data->MLMin; MLMax = data->MLMax;

	TestDB.PGlobal       = 1;
	TestDB.PG_add        = data->PG_add;
	TestDB.IntOrder_add  = data->IntOrder_add;
	TestDB.IntOrder_mult = data->IntOrder_mult;

	mesh_quality = malloc((MLMax-MLMin+1) * sizeof *mesh_quality); // free

	if (Adapt != ADAPT_0) {
		TestDB.ML = DB.ML;
		code_startup(nargc,argvNew,0,2);
	}

	for (size_t P = PMin; P <= PMax; P++) {
	for (size_t ML = MLMin; ML <= MLMax; ML++) {
		TestDB.PGlobal = P;
		TestDB.ML = ML;

		if (Adapt != ADAPT_0) {
			if (ML == MLMin) {
				mesh_to_level(ML);
				if (AdaptiveRefine)
					h_adapt_test();
			} else {
				mesh_h_adapt(1,'r');
			}
			mesh_to_order(TestDB.PGlobal);
		} else {
			code_startup(nargc,argvNew,0,1);
		}

		if (Compute_L2proj) { // Compute errors of L2 projection of the exact solution
			char *string, *fNameOut;

			fNameOut = malloc(STRLEN_MAX * sizeof *fNameOut); // free
			string   = malloc(STRLEN_MIN * sizeof *string);   // free

			initialize_test_case(0);
			// Output to paraview
			if (TestDB.ML <= 1 || (TestDB.PGlobal == 1) || (TestDB.PGlobal == 5 && TestDB.ML <= 4)) {
				fNameOut = get_fNameOut("SolFinal_");
				output_to_paraview(fNameOut);

				if (TestDB.PGlobal == 5 && TestDB.ML <= 2) {
					free(fNameOut); fNameOut = get_fNameOut("MeshEdges_");
					output_to_paraview(fNameOut);
				}
			}

			free(fNameOut);
			free(string);
		} else {
			solver_Poisson(PrintEnabled);
		}
		compute_errors_global();

		if (PrintEnabled)
			printf("dof: %d\n",DB.dof);

		if (P == PMin)
			evaluate_mesh_regularity(&mesh_quality[ML-MLMin]);

		if (P == PMax && ML == MLMax) {
			check_convergence_orders(MLMin,MLMax,PMin,PMax,&pass,PrintEnabled);
			check_mesh_regularity(mesh_quality,MLMax-MLMin+1,&pass,PrintEnabled);
		}

		if (Adapt == ADAPT_0) {
			set_PrintName_ConvOrders(data->PrintName,&data->TestTRI);
			code_cleanup();
		}
	}}
	if (Adapt != ADAPT_0) {
		set_PrintName_ConvOrders(data->PrintName,&data->TestTRI);
		code_cleanup();
	}
	free(mesh_quality);

	test_print2(pass,data->PrintName);
}

void test_integration_Poisson(int nargc, char **argv)
{
	char **argvNew, *TestName, *PrintName;

	argvNew    = malloc(2          * sizeof *argvNew);  // free
	argvNew[0] = malloc(STRLEN_MAX * sizeof **argvNew); // free
	argvNew[1] = malloc(STRLEN_MAX * sizeof **argvNew); // free
	TestName   = malloc(STRLEN_MAX * sizeof *TestName); // free
	PrintName  = malloc(STRLEN_MAX * sizeof *PrintName); // free

	strcpy(argvNew[0],argv[0]);

	/*
	 *	Input:
	 *
	 *		Meshes for a curved Poisson problem.
	 *
	 *	Expected Output:
	 *
	 *		Correspondence of LHS matrices computed using complex step and exact linearization.
	 *		Optimal convergence orders in L2 for the solution (P+1) and its gradients (P).
	 *
	 */

	struct S_linearization *data_l;
	struct S_convorder     *data_c;

	data_l = calloc(1 , sizeof *data_l); // free
	data_c = calloc(1 , sizeof *data_c); // free

	data_c->argvNew   = argvNew;
	data_c->PrintName = PrintName;


	// **************************************************************************************************** //
	// Linearization Testing
	// **************************************************************************************************** //
	TestDB.PG_add = 0;
	TestDB.IntOrder_mult = 2;

	// **************************************************************************************************** //
	// 2D (Mixed TRI/QUAD mesh)
	TestDB.PGlobal = 2;
	TestDB.ML      = 0;

	//              0         10        20        30        40        50
	strcpy(TestName,"Linearization Poisson (2D - Mixed):              ");
	strcpy(argvNew[1],"test/Poisson/Test_Poisson_n-Ball_HollowSection_CurvedMIXED2D");

//if (0)
	test_linearization(nargc,argvNew,2,1,TestName,data_l);


	// **************************************************************************************************** //
	// 3D (TET mesh)
	TestDB.PGlobal = 2;
	TestDB.ML      = 0;

	//              0         10        20        30        40        50
	strcpy(TestName,"Linearization Poisson (3D - TET):                ");
	strcpy(argvNew[1],"test/Test_Poisson_3D_TET");

if (0) // The 3D testing needs to be updated (ToBeDeleted)
	test_linearization(nargc,argvNew,0,1,TestName,data_l);


	// **************************************************************************************************** //
	// Convergence Order Testing
	// **************************************************************************************************** //
//	strcpy(argvNew[1],"test/Test_Poisson_dm1-Spherical_Section_2D_mixed");
//	strcpy(argvNew[1],"test/Test_Poisson_dm1-Spherical_Section_2D_TRI");
//	strcpy(argvNew[1],"test/Test_Poisson_Ellipsoidal_Section_2D_TRI");
//	strcpy(argvNew[1],"test/Test_Poisson_Ellipsoidal_Section_2D_QUAD");
//	strcpy(argvNew[1],"test/Test_Poisson_Ringleb2D_TRI");
//	strcpy(argvNew[1],"test/Test_Poisson_Ringleb2D_QUAD");
//	strcpy(argvNew[1],"test/Test_Poisson_HoldenRamp2D_TRI");
//	strcpy(argvNew[1],"test/Test_Poisson_GaussianBump2D_TRI");
//	strcpy(argvNew[1],"test/Test_Poisson_GaussianBump2D_mixed");
//	strcpy(argvNew[1],"test/Test_Poisson_3D_TET");
//	strcpy(argvNew[1],"test/Test_Poisson_3D_HEX");
//	strcpy(argvNew[1],"test/Test_Poisson_mixed3D_TP");
//	strcpy(argvNew[1],"test/Test_Poisson_mixed3D_HW");

	// Add in test cases from above if they are used in Euler/NS verification cases (ToBeModified)
	test_convorder(nargc,argvNew,"n-Ellipsoid_HollowSection_TRI",data_c);
	test_convorder(nargc,argvNew,"n-Ellipsoid_HollowSection_QUAD",data_c);


	array_free2_c(2,argvNew);
	free(TestName);
	free(PrintName);
	free(data_l);
	free(data_c);
}
