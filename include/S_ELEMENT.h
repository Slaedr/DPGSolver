// Copyright 2016 Philip Zwanenburg
// MIT License (https://github.com/PhilipZwanenburg/DPGSolver/master/LICENSE)

#ifndef DPG__S_ELEMENT_h__INCLUDED
#define DPG__S_ELEMENT_h__INCLUDED

#include "S_OpCSR.h"

struct S_ELEMENT {
	// Mesh
	unsigned int present, type, d, Nve, Nf, Nvref, NvrefSF, Eclass, NEhref,
	             *Nfve, *VeCGmsh, *VeFcon, *NrefV, *type_h;

	// Operators
	unsigned int *connect_NE, *NvnP, *Nvve,
	             *NvnGs, *NvnGc, *NvnCs, *NvnCc, *NvnJs, *NvnJc, *NvnIs, *NvnIc, *NvnS,
	             **NfnS, **NfnIs, **NfnIc,
	             *Nfref, *NfMixed,
	             **connectivity, **connect_types,
	             ***nOrd_fS, ***nOrd_fIs, ***nOrd_fIc;
	double       **VeF, **VeV, *nr,
	             **w_vIs, **w_vIc,
	             ****ChiS_vP, ****ChiS_vS, ****ChiS_vIs, ****ChiS_vIc,
	             ****ChiInvS_vS,
	             ****ICs, ****ICc,
	             ****I_vGs_vP, ****I_vGs_vGs, ****I_vGs_vGc, ****I_vGs_vCs, ****I_vGs_vS, ****I_vGs_vIs, ****I_vGs_vIc,
	             ****I_vGc_vP,                               ****I_vGc_vCc, ****I_vGc_vS, ****I_vGc_vIs, ****I_vGc_vIc,
	             ****I_vCs_vS, ****I_vCs_vIs, ****I_vCs_vIc,
	             ****I_vCc_vS, ****I_vCc_vIs, ****I_vCc_vIc,
	             ****Ihat_vS_vS,
	             *****D_vGs_vCs, *****D_vGs_vIs,
	             *****D_vGc_vCc, *****D_vGc_vIc,
	             *****D_vCs_vCs,
	             *****D_vCc_vCc,
	             ****ChiS_fS, ****ChiS_fIs, ****ChiS_fIc,
				 *****GradChiS_fIs, *****GradChiS_fIc,
	             ****I_vGs_fS, ****I_vGs_fIs, ****I_vGs_fIc,
	             ****I_vGc_fS, ****I_vGc_fIs, ****I_vGc_fIc,
	             ****I_vCs_fS, ****I_vCs_fIs, ****I_vCs_fIc,
	             ****I_vCc_fS, ****I_vCc_fIs, ****I_vCc_fIc,
				 *****D_vGs_fIs, *****D_vGs_fIc,
				 *****D_vGc_fIs, *****D_vGc_fIc,
	             ****Is_Weak_VV, ****Ic_Weak_VV,
	             ****Is_Weak_FF, ****Ic_Weak_FF,
	             *****Ds_Weak_VV, *****Dc_Weak_VV,
	             ****L2hat_vS_vS,
	             ****GfS_fIs, ****GfS_fIc;

	struct S_OpCSR ****ChiS_fIs_sp, ****ChiS_fIc_sp,
	               *****Ds_Weak_VV_sp, *****Dc_Weak_VV_sp,
	               ****Is_Weak_FF_sp, ****Ic_Weak_FF_sp;

	struct S_ELEMENT *next;
	struct S_ELEMENT **ELEMENTclass, **ELEMENT_FACET;
};

#endif // DPG__S_ELEMENT_h__INCLUDED