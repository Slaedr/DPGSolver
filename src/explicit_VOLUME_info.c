// Copyright 2016 Philip Zwanenburg
// MIT License (https://github.com/PhilipZwanenburg/DPGSolver/master/LICENSE)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "database.h"
#include "parameters.h"
#include "functions.h"

/*
 *	Purpose:
 *		Evaluate the VOLUME contributions to the RHS term.
 *
 *	Comments:
 *		Need to add in support for sum factorization when working (ToBeDeleted).
 *		Compared with Matlab code on TRIs => Identical results. (ToBeDeleted)
 *
 *	Notation:
 *
 *	References:
 */

struct S_OPERATORS {
	unsigned int NvnI, NvnS, NvnS_SF, NvnI_SF;
	double       *ChiS_vI, **D_Weak, *I_Weak;
};

static void init_ops(struct S_OPERATORS *OPS, const struct S_VOLUME *VOLUME, const unsigned int IndClass);
static void compute_VOLUME_RHS_EFE(void);
static void compute_VOLUMEVec_RHS_EFE(void);

void explicit_VOLUME_info(void)
{
	// Initialize DB Parameters
	unsigned int EFE        = DB.EFE,
	             Vectorized = DB.Vectorized;

	if (EFE) {
		switch (Vectorized) {
		case 0:
			compute_VOLUME_RHS_EFE();
			break;
		default:
			compute_VOLUMEVec_RHS_EFE();
			break;
		}
	} else {
		;
	}
}

static void init_ops(struct S_OPERATORS *OPS, const struct S_VOLUME *VOLUME, const unsigned int IndClass)
{
	// Initialize DB Parameters
	unsigned int ***SF_BE = DB.SF_BE;

	// Standard datatypes
	unsigned int P, type, curved, Eclass;
	struct S_ELEMENT *ELEMENT, *ELEMENT_OPS;

	// silence
	ELEMENT_OPS = NULL;

	P      = VOLUME->P;
	type   = VOLUME->type;
	curved = VOLUME->curved;
	Eclass = VOLUME->Eclass;

	ELEMENT = get_ELEMENT_type(type);
	if ((Eclass == C_TP && SF_BE[P][0][0]) || (Eclass == C_WEDGE && SF_BE[P][1][0]))
		ELEMENT_OPS = ELEMENT->ELEMENTclass[IndClass];
//	else if (Eclass == C_TP || Eclass == C_SI || Eclass == C_PYR || Eclass == C_WEDGE)
	else
		ELEMENT_OPS = ELEMENT;

	OPS->NvnS    = ELEMENT->NvnS[P];
	OPS->NvnS_SF = ELEMENT_OPS->NvnS[P];
	if (!curved) {
		OPS->NvnI    = ELEMENT->NvnIs[P];
		OPS->NvnI_SF = ELEMENT_OPS->NvnIs[P];

		OPS->ChiS_vI = ELEMENT_OPS->ChiS_vIs[P][P][0];
		OPS->D_Weak  = ELEMENT_OPS->Ds_Weak_VV[P][P][0];
		OPS->I_Weak  = ELEMENT_OPS->Is_Weak_VV[P][P][0];
	} else {
		OPS->NvnI    = ELEMENT->NvnIc[P];
		OPS->NvnI_SF = ELEMENT_OPS->NvnIc[P];

		OPS->ChiS_vI = ELEMENT_OPS->ChiS_vIc[P][P][0];
		OPS->D_Weak  = ELEMENT_OPS->Dc_Weak_VV[P][P][0];
		OPS->I_Weak  = ELEMENT_OPS->Ic_Weak_VV[P][P][0];
	}
}

static void compute_VOLUME_RHS_EFE(void)
{
	// Initialize DB Parameters
	char         *Form = DB.Form;
	unsigned int d          = DB.d,
	             Collocated = DB.Collocated,
				 Nvar       = DB.Nvar,
				 Neq        = DB.Neq,
	             ***SF_BE   = DB.SF_BE;

	// Standard datatypes
	unsigned int i, eq, dim1, dim2, P,
	             IndFr, IndF, IndC, IndRHS, Eclass,
	             NvnI, NvnS, NvnI_SF[2], NvnS_SF[2], NIn[3], NOut[3], Diag[3];
	double       *W_vI, *F_vI, *Fr_vI, *C_vI, *RHS, *DFr, **D, *I, *OP[3];

	struct S_OPERATORS *OPS[2];
	struct S_VOLUME    *VOLUME;

	for (i = 0; i < 2; i++)
		OPS[i] = malloc(sizeof *OPS[i]); // free

	if (strstr(Form,"Weak") != NULL) {
		for (VOLUME = DB.VOLUME; VOLUME != NULL; VOLUME = VOLUME->next) {
			P = VOLUME->P;
//printf("VOLUME: %d\n",VOLUME->indexg);
			// Obtain operators
			init_ops(OPS[0],VOLUME,0);
			if (VOLUME->type == WEDGE)
				init_ops(OPS[1],VOLUME,1);

			Eclass = VOLUME->Eclass;

			// Obtain W_vI
			NvnI = OPS[0]->NvnI;
			if (Collocated) {
				W_vI = VOLUME->What;
			} else {
				W_vI = malloc(NvnI*Nvar * sizeof *W_vI); // free
				
				if (Eclass == C_TP && SF_BE[P][0][0]) {
					for (i = 0; i < 1; i++) {
						NvnS_SF[i] = OPS[i]->NvnS_SF;
						NvnI_SF[i] = OPS[i]->NvnI_SF;
					}
					get_sf_parameters(NvnS_SF[0],NvnI_SF[0],OPS[0]->ChiS_vI,0,0,NULL,NIn,NOut,OP,d,3,Eclass);

					if (Collocated) {
						for (dim2 = 0; dim2 < d; dim2++)
							Diag[dim2] = 1;
					} else {
						for (dim2 = 0; dim2 < d; dim2++)
							Diag[dim2] = 0;
					}
					sf_apply_d(VOLUME->What,W_vI,NIn,NOut,Nvar,OP,Diag,d);
				} else if (Eclass == C_WEDGE && SF_BE[P][1][0]) {

				} else {
					mm_CTN_d(NvnI,Nvar,OPS[0]->NvnS,OPS[0]->ChiS_vI,VOLUME->What,W_vI);
				}
			}
//array_print_d(NvnI,Neq,W_vI,'C');

			// Compute Flux in reference space
			F_vI = malloc(NvnI*d*Neq * sizeof *F_vI); // free
			flux_inviscid(NvnI,1,W_vI,F_vI,d,Neq);

			if (!Collocated)
				free(W_vI);

			C_vI = VOLUME->C_vI;

			Fr_vI = calloc(NvnI*d*Neq , sizeof *Fr_vI); // free
			for (eq = 0; eq < Neq; eq++) {
			for (dim1 = 0; dim1 < d; dim1++) {
			for (dim2 = 0; dim2 < d; dim2++) {
				IndFr = (eq*d+dim1)*NvnI;
				IndF  = (eq*d+dim2)*NvnI;
				IndC  = (dim1*d+dim2)*NvnI;
				for (i = 0; i < NvnI; i++)
					Fr_vI[IndFr+i] += F_vI[IndF+i]*C_vI[IndC+i];
			}}}
			free(F_vI);

//array_print_d(NvnI,d*d,C_vI,'C');

for (eq = 0; eq < Neq; eq++) {
//array_print_d(NvnI,d,&Fr_vI[NvnI*d*eq],'C');
}

			// Compute RHS terms
			NvnS = OPS[0]->NvnS;

			RHS = calloc(NvnS*Neq , sizeof *RHS); // keep (requires external free)
			VOLUME->RHS = RHS;
			if (Eclass == C_TP && SF_BE[P][0][0]) {
				DFr = malloc(NvnS * sizeof *DFr); // free

				for (i = 0; i < 1; i++) {
					NvnS_SF[i] = OPS[i]->NvnS_SF;
					NvnI_SF[i] = OPS[i]->NvnI_SF;
				}

				I = OPS[0]->I_Weak;
				D = OPS[0]->D_Weak;

				for (dim1 = 0; dim1 < d; dim1++) {
					get_sf_parameters(NvnI_SF[0],NvnS_SF[0],I,NvnI_SF[0],NvnS_SF[0],D[0],NIn,NOut,OP,d,dim1,Eclass);

					if (Collocated) {
						for (dim2 = 0; dim2 < d; dim2++)
							Diag[dim2] = 1;
						Diag[dim1] = 0;
					} else {
						for (dim2 = 0; dim2 < d; dim2++)
							Diag[dim2] = 0;
					}

					for (eq = 0; eq < Neq; eq++) {
						sf_apply_d(&Fr_vI[(eq*d+dim1)*NvnI],DFr,NIn,NOut,1,OP,Diag,d);

//if (eq == 0 && dim1 == 0) {
//	array_print_d(NvnS,1,DFr,'C');
//}

						IndRHS = eq*NvnS;
						for (i = 0; i < NvnS; i++)
							RHS[IndRHS+i] += DFr[i];
					}
				}
				free(DFr);
			} else if (Eclass == C_WEDGE && SF_BE[P][1][0]) {
				; // update this with sum factorization
			} else {
				DFr = malloc(NvnS * sizeof *DFr); // free
				D = OPS[0]->D_Weak;

				for (eq = 0; eq < Neq; eq++) {
				for (dim1 = 0; dim1 < d; dim1++) {
					mm_CTN_d(NvnS,1,NvnI,D[dim1],&Fr_vI[(eq*d+dim1)*NvnI],DFr);
/*
if (eq == 0 && dim1 == 0) {
	array_print_d(NvnI,1,&Fr_vI[(eq*d+dim1)*NvnI],'C');
	array_print_d(NvnS,1,DFr,'C');
}
*/
					IndRHS = eq*NvnS;
					for (i = 0; i < NvnS; i++)
						RHS[IndRHS+i] += DFr[i];
				}}
				free(DFr);

			}
			free(Fr_vI);
/*
printf("%d %d %d\n",NvnS,NvnI,SF_BE[P][0][0]);
array_print_d(NvnS,Nvar,RHS,'C');
exit(1);
*/
		}
	} else if (strstr(Form,"Strong") != NULL) {
		printf("Exiting: Implement the strong form in compute_VOLUME_RHS_EFE.\n"), exit(1);
	}
//exit(1);

	for (i = 0; i < 2; i++)
		free(OPS[i]);
}

static void compute_VOLUMEVec_RHS_EFE(void)
{

}
