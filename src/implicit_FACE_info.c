// Copyright 2017 Philip Zwanenburg
// MIT License (https://github.com/PhilipZwanenburg/DPGSolver/blob/master/LICENSE)

#include "implicit_FACE_info.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Parameters.h"
#include "Macros.h"
#include "S_DB.h"
#include "S_FACE.h"

#include "solver_functions.h"
#include "array_free.h"


/*
 *	Purpose:
 *		Evaluate the FACE contributions to the RHS and LHS terms.
 *
 *	Comments:
 *
 *	Notation:
 *
 *	References:
 */

static void compute_Inviscid_FACE_EFE (void);
static void compute_Viscous_FACE_EFE (void);
static void compute_Viscous_VOLUME_FACE_EFE(void);

void implicit_FACE_info(void)
{
	compute_Inviscid_FACE_EFE();
	compute_Viscous_FACE_EFE();
}

void implicit_FACE_Q_info(void)
{
	compute_Viscous_VOLUME_FACE_EFE();
}

static void compute_Inviscid_FACE_EFE(void)
{
	unsigned int const d    = DB.d,
	                   Nvar = d+2,
	                   Neq  = d+2;

	struct S_OPERATORS_F *OPSL[2], *OPSR[2];
	struct S_FACE     *FACE;

	struct S_FDATA *FDATAL = malloc(sizeof *FDATAL), // free
	               *FDATAR = malloc(sizeof *FDATAR); // free
	FDATAL->OPS = (struct S_OPERATORS_F const *const *) OPSL;
	FDATAR->OPS = (struct S_OPERATORS_F const *const *) OPSR;

	struct S_NumericalFlux *NFluxData = malloc(sizeof *NFluxData); // free
	FDATAL->NFluxData = NFluxData;
	FDATAR->NFluxData = NFluxData;

	for (size_t i = 0; i < 2; i++) {
		OPSL[i] = malloc(sizeof *OPSL[i]); // free
		OPSR[i] = malloc(sizeof *OPSR[i]); // free
	}

	if (strstr(DB.Form,"Weak")) {
		for (FACE = DB.FACE; FACE; FACE = FACE->next) {
			init_FDATA(FDATAL,FACE,'L');
			init_FDATA(FDATAR,FACE,'R');

			// Compute WL_fIL and WR_fIL (i.e. as seen from the (L)eft VOLUME)
			unsigned int const IndFType = FDATAL->IndFType,
			                   NfnI     = OPSL[IndFType]->NfnI;

			FDATAL->W_fIL = malloc(NfnI*Nvar * sizeof *(FDATAL->W_fIL)), // free
			FDATAR->W_fIL = malloc(NfnI*Nvar * sizeof *(FDATAR->W_fIL)); // free

			coef_to_values_fI(FDATAL,'W',0);
			compute_WR_fIL(FDATAR,FDATAL->W_fIL,FDATAR->W_fIL);


			// Compute numerical flux and its Jacobian as seen from the left VOLUME
			NFluxData->WL_fIL          = FDATAL->W_fIL;
			NFluxData->WR_fIL          = FDATAR->W_fIL;
			NFluxData->nFluxNum_fI     = malloc(NfnI*Neq      * sizeof *(NFluxData->nFluxNum_fI));     // free
			NFluxData->dnFluxNumdWL_fI = malloc(NfnI*Neq*Nvar * sizeof *(NFluxData->dnFluxNumdWL_fI)); // free
			NFluxData->dnFluxNumdWR_fI = malloc(NfnI*Neq*Nvar * sizeof *(NFluxData->dnFluxNumdWR_fI)); // free

			compute_numerical_flux(FDATAL,'I');
			add_Jacobian_scaling_FACE(FDATAL,'I','W');

			free(FDATAL->W_fIL);
			free(FDATAR->W_fIL);


			// Compute FACE RHS and LHS terms
			unsigned int NvnSL = OPSL[0]->NvnS,
			             NvnSR = OPSR[0]->NvnS;

			memset(FACE->RHSIn,0.0,NvnSL*Neq * sizeof *(FACE->RHSIn));
			finalize_FACE_Inviscid_Weak(FDATAL,FDATAR,NFluxData->nFluxNum_fI,NULL,'L','E','W');

			memset(FACE->LHSInIn,0.0,NvnSL*NvnSL*Neq*Nvar * sizeof *(FACE->LHSInIn));
			finalize_FACE_Inviscid_Weak(FDATAL,FDATAR,NFluxData->dnFluxNumdWL_fI,NFluxData->dnFluxNumdWR_fI,'L','I','W');

			if (!FACE->Boundary) {
				memset(FACE->RHSOut,0.0,NvnSR*Neq * sizeof *(FACE->RHSOut));
				finalize_FACE_Inviscid_Weak(FDATAL,FDATAR,NFluxData->nFluxNum_fI,NULL,'R','E','W');

				memset(FACE->LHSOutIn,0.0,NvnSL*NvnSR*Neq*Nvar * sizeof *(FACE->LHSOutIn));
				memset(FACE->LHSInOut,0.0,NvnSR*NvnSL*Neq*Nvar * sizeof *(FACE->LHSInOut));
				memset(FACE->LHSOutOut,0.0,NvnSR*NvnSR*Neq*Nvar * sizeof *(FACE->LHSOutOut));
				finalize_FACE_Inviscid_Weak(FDATAL,FDATAR,NFluxData->dnFluxNumdWL_fI,NFluxData->dnFluxNumdWR_fI,'R','I','W');
			}
			free(NFluxData->nFluxNum_fI);
			free(NFluxData->dnFluxNumdWL_fI);
			free(NFluxData->dnFluxNumdWR_fI);
		}
	} else if (strstr(DB.Form,"Strong")) {
		EXIT_UNSUPPORTED;
	}

	for (size_t i = 0; i < 2; i++) {
		free(OPSL[i]);
		free(OPSR[i]);
	}

	free(FDATAL);
	free(FDATAR);
	free(NFluxData);
}

static void compute_Viscous_FACE_EFE(void)
{
	/*
	 *	Comments:
	 *		It is currently uncertain whether the local or partially corrected weak gradient should be used to compute
	 *		the boundary conditions here. Using the local contribution QhatV gave good results during initial testing
	 *		but adding the partial correction is perhaps more consistent with how the viscous numerical fluxes are
	 *		evaluated. Currently only the use of the local weak gradient is supported. Investigate (ToBeModified).
	 *
	 *		Bassi(2005) appears to use the corrected gradient for the boundary condition evaluation as well (See eq.
	 *		(77) and expanded terms).
	 *
	 *	References:
	 *		Bassi(2005)-Discontinuous_Galerkin_Solution_of_the_Reynolds-Averaged_Navier-Stokes_and_k-w_Turbulence_Model_
	 *		            Equations
	 */

	if (!DB.Viscous)
		return;

	bool const CorrectedGradW = 1;

	unsigned int const d    = DB.d,
	                   Nvar = d+2,
	                   Neq  = d+2;

	struct S_OPERATORS_F *OPSL[2], *OPSR[2];
	struct S_FACE     *FACE;

	struct S_FDATA *FDATAL = malloc(sizeof *FDATAL), // free
	               *FDATAR = malloc(sizeof *FDATAR); // free
	FDATAL->OPS = (struct S_OPERATORS_F const *const *) OPSL;
	FDATAR->OPS = (struct S_OPERATORS_F const *const *) OPSR;

	struct S_NumericalFlux *NFluxData = malloc(sizeof *NFluxData); // free
	FDATAL->NFluxData = NFluxData;
	FDATAR->NFluxData = NFluxData;

	for (size_t i = 0; i < 2; i++) {
		OPSL[i] = malloc(sizeof *OPSL[i]); // free
		OPSR[i] = malloc(sizeof *OPSR[i]); // free
	}

	if (strstr(DB.Form,"Weak")) {
		for (FACE = DB.FACE; FACE; FACE = FACE->next) {
			init_FDATA(FDATAL,FACE,'L');
			init_FDATA(FDATAR,FACE,'R');

			// Compute WL_fIL and WR_fIL (i.e. as seen from the (L)eft VOLUME)
			unsigned int const IndFType = FDATAL->IndFType,
			                   NfnI     = OPSL[IndFType]->NfnI;

			FDATAL->W_fIL = malloc(NfnI*Nvar * sizeof *(FDATAL->W_fIL)), // free
			FDATAR->W_fIL = malloc(NfnI*Nvar * sizeof *(FDATAR->W_fIL)); // free

			double **const GradWL_fIL = malloc(d * sizeof *GradWL_fIL), // free
			       **const GradWR_fIL = malloc(d * sizeof *GradWR_fIL); // free

			for (size_t dim = 0; dim < d; dim++) {
				GradWL_fIL[dim] = malloc(NfnI*Nvar * sizeof *GradWL_fIL[dim]); // free
				GradWR_fIL[dim] = malloc(NfnI*Nvar * sizeof *GradWR_fIL[dim]); // free
			}

			FDATAL->GradW_fIL = GradWL_fIL;
			FDATAR->GradW_fIL = GradWR_fIL;

			coef_to_values_fI(FDATAL,'W',0);
			coef_to_values_fI(FDATAL,'Q',CorrectedGradW);
			compute_WR_GradWR_fIL(FDATAR,FDATAL->W_fIL,FDATAR->W_fIL,(double const *const *const) GradWL_fIL,GradWR_fIL,
			                      CorrectedGradW);


			// Compute numerical flux and its Jacobian as seen from the left VOLUME
			NFluxData->WL_fIL              = FDATAL->W_fIL;
			NFluxData->WR_fIL              = FDATAR->W_fIL;
			NFluxData->nFluxViscNum_fI     = malloc(NfnI*Neq      * sizeof *(NFluxData->nFluxViscNum_fI));     // free
			NFluxData->dnFluxViscNumdWL_fI = malloc(NfnI*Neq*Nvar * sizeof *(NFluxData->dnFluxViscNumdWL_fI)); // free
			NFluxData->dnFluxViscNumdWR_fI = malloc(NfnI*Neq*Nvar * sizeof *(NFluxData->dnFluxViscNumdWR_fI)); // free

			compute_numerical_flux_viscous(FDATAL,FDATAR,'I');
			add_Jacobian_scaling_FACE(FDATAL,'I','V');

			free(FDATAL->W_fIL);
			free(FDATAR->W_fIL);
			array_free2_d(d,GradWL_fIL);
			array_free2_d(d,GradWR_fIL);


			// Compute FACE RHS and LHS terms
			finalize_FACE_Viscous_Weak(FDATAL,FDATAR,NFluxData->nFluxViscNum_fI,NULL,'L','E','V');
			finalize_FACE_Viscous_Weak(FDATAL,FDATAR,NFluxData->dnFluxViscNumdWL_fI,NFluxData->dnFluxViscNumdWR_fI,'L','I','V');

			if (!FACE->Boundary) {
				finalize_FACE_Viscous_Weak(FDATAL,FDATAR,NFluxData->nFluxViscNum_fI,NULL,'R','E','V');
				finalize_FACE_Viscous_Weak(FDATAL,FDATAR,NFluxData->dnFluxViscNumdWL_fI,NFluxData->dnFluxViscNumdWR_fI,'R','I','V');
			}
			free(NFluxData->nFluxViscNum_fI);
			free(NFluxData->dnFluxViscNumdWL_fI);
			free(NFluxData->dnFluxViscNumdWR_fI);
		}
	} else if (strstr(DB.Form,"Strong")) {
		EXIT_UNSUPPORTED;
	}

	for (size_t i = 0; i < 2; i++) {
		free(OPSL[i]);
		free(OPSR[i]);
	}

	free(FDATAL);
	free(FDATAR);
	free(NFluxData);
}

static void compute_Viscous_VOLUME_FACE_EFE(void)
{
	/*
	 *	Comments:
	 *		It is assumed that VOLUME->LHSQ has been computed and that VL->LHS, VR->LHS and FACE->LHS(RL/LR) have been
	 *		initialized.
	 */

	if (!DB.Viscous)
		return;

	if (strstr(DB.Form,"Weak")) {
		for (struct S_FACE *FACE = DB.FACE; FACE; FACE = FACE->next)
			finalize_VOLUME_LHSQF_Weak(FACE);
	} else if (strstr(DB.Form,"Strong")) {
		EXIT_UNSUPPORTED;
	} else {
		EXIT_UNSUPPORTED;
	}
}
