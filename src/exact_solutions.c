// Copyright 2017 Philip Zwanenburg
// MIT License (https://github.com/PhilipZwanenburg/DPGSolver/blob/master/LICENSE)

#include "exact_solutions.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "Parameters.h"
#include "Macros.h"
#include "S_DB.h"

/*
 *	Purpose:
 *		Provide functions related to exact solutions.
 *
 *	Comments:
 *
 *	Notation:
 *
 *	References:
 */

void compute_exact_solution(const unsigned int Nn, double *XYZ, double *UEx, const unsigned int solved)
{
	// Initialize DB Parameters
	char         *TestCase = DB.TestCase;
	unsigned int d         = DB.d;

	// Standard datatypes
	unsigned int i;
	double       *X, *Y, *Z, *rhoEx, *uEx, *vEx, *wEx, *pEx;

	rhoEx = &UEx[Nn*0];
	uEx   = &UEx[Nn*1];
	vEx   = &UEx[Nn*2];
	wEx   = &UEx[Nn*3];
	pEx   = &UEx[Nn*4];

	X = &XYZ[0*Nn];
	Y = &XYZ[1*Nn];
	Z = &XYZ[(d-1)*Nn];

	// Perhaps modify TestCase for Test_L2_proj and Test_update_h to make this cleaner, also in other functions (ToBeDeleted)
	if (strstr(TestCase,"PeriodicVortex")) {
		// Initialize DB Parameters
		double       pInf           = DB.pInf,
					 TInf           = DB.TInf,
					 VInf           = DB.VInf,
					 uInf           = DB.uInf,
					 vInf           = DB.vInf,
					 wInf           = DB.wInf,
					 Rg             = DB.Rg,
					 Cscale         = DB.Cscale,
					 PeriodL        = DB.PeriodL,
					 PeriodFraction = DB.PeriodFraction,
					 Xc0            = DB.Xc,
					 Yc             = DB.Yc,
					 Rc             = DB.Rc;

		// Standard datatypes
		double DistTraveled, Xc, rhoInf, r2, C;

		rhoInf = pInf/(Rg*TInf);
		C      = Cscale*VInf;

		if (solved) {
			DistTraveled = PeriodL*PeriodFraction;
			Xc = Xc0 + DistTraveled;
			while (Xc > 0.5*PeriodL)
				Xc -= PeriodL;
		} else {
			Xc = Xc0;
		}

		for (i = 0; i < Nn; i++) {
			r2 = (pow(X[i]-Xc,2.0)+pow(Y[i]-Yc,2.0))/(Rc*Rc);

			uEx[i]   = uInf - C*(Y[i]-Yc)/(Rc*Rc)*exp(-0.5*r2);
			vEx[i]   = vInf + C*(X[i]-Xc)/(Rc*Rc)*exp(-0.5*r2);
			wEx[i]   = wInf;
			pEx[i]   = pInf - rhoInf*(C*C)/(2*Rc*Rc)*exp(-r2);
			rhoEx[i] = rhoInf;
		}
	} else if (strstr(TestCase,"SupersonicVortex")) {
		// Initialize DB Parameters
		double rIn   = DB.rIn,
		       MIn   = DB.MIn,
		       rhoIn = DB.rhoIn,
		       VIn   = DB.VIn;

		// Standard datatypes
		double r, t, Vt;

		for (i = 0; i < Nn; i++) {
			r = sqrt(X[i]*X[i]+Y[i]*Y[i]);
			t = atan2(Y[i],X[i]);

			rhoEx[i] = rhoIn*pow(1.0+0.5*GM1*MIn*MIn*(1.0-pow(rIn/r,2.0)),1.0/GM1);
			pEx[i]   = pow(rhoEx[i],GAMMA)/GAMMA;

			Vt = -VIn/r;
			uEx[i] = -sin(t)*Vt;
			vEx[i] =  cos(t)*Vt;
			wEx[i] =  0.0;
		}
	} else if (strstr(TestCase,"Poisson")) {
		double Poisson_scale = DB.Poisson_scale;

		if (fabs(Poisson_scale) < EPS)
			printf("Error: Make sure to set Poisson_scale.\n"), EXIT_MSG;

		for (i = 0; i < Nn; i++) {
			if (d == 2)
//				UEx[i] = sin(PI*X[i])*sin(PI*Y[i]);
				UEx[i] = cos(Poisson_scale*PI*X[i])*cos(Poisson_scale*PI*Y[i]);
//				UEx[i] = X[i]*Y[i]*sin(PI*X[i])*sin(PI*Y[i]);
			else if (d == 3)
				UEx[i] = sin(PI*X[i])*sin(PI*Y[i])*sin(PI*Z[i]);
		}
	} else {
		printf("Error: Unsupported TestCase.\n"), EXIT_MSG;
	}
}

void compute_exact_gradient(const unsigned int Nn, double *XYZ, double *QEx)
{
	// Initialize DB Parameters
	char         *TestCase = DB.TestCase;
	unsigned int d         = DB.d;

	// Standard datatypes
	unsigned int i;
	double       *X, *Y, *Z;

	X = &XYZ[0*Nn];
	Y = &XYZ[1*Nn];
	Z = &XYZ[(d-1)*Nn];

	if (strstr(TestCase,"Poisson")) {
		double Poisson_scale = DB.Poisson_scale;
		for (i = 0; i < Nn; i++) {
			if (d == 2) {
//				QEx[Nn*0+i] = PI*cos(PI*X[i])*sin(PI*Y[i]);
//				QEx[Nn*1+i] = PI*sin(PI*X[i])*cos(PI*Y[i]);
				QEx[Nn*0+i] = -Poisson_scale*PI*sin(Poisson_scale*PI*X[i])*cos(Poisson_scale*PI*Y[i]);
				QEx[Nn*1+i] = -Poisson_scale*PI*cos(Poisson_scale*PI*X[i])*sin(Poisson_scale*PI*Y[i]);
//				QEx[Nn*0+i] = Y[i]*sin(PI*Y[i])*(sin(PI*X[i])+X[i]*PI*cos(PI*X[i]));
//				QEx[Nn*1+i] = X[i]*sin(PI*X[i])*(sin(PI*Y[i])+Y[i]*PI*cos(PI*Y[i]));
			} else if (d == 3) {
				QEx[Nn*0+i] = PI*cos(PI*X[i])*sin(PI*Y[i])*sin(PI*Z[i]);
				QEx[Nn*1+i] = PI*sin(PI*X[i])*cos(PI*Y[i])*sin(PI*Z[i]);
				QEx[Nn*2+i] = PI*sin(PI*X[i])*sin(PI*Y[i])*cos(PI*Z[i]);
			}
		}
	} else {
		printf("Error: Unsupported TestCase.\n"), EXIT_MSG;
	}
}

void compute_source(const unsigned int Nn, double *XYZ, double *source)
{
	/*
	 *	Purpose:
	 *		Computes source terms.
	 */

	// Initialize DB Parameters
	char         *TestCase = DB.TestCase;
	unsigned int d         = DB.d,
	             Neq       = DB.Neq;

	// Standard datatypes
	unsigned int n, eq;
	double       *X, *Y, *Z;

	if (strstr(TestCase,"Poisson")) {
		double Poisson_scale = DB.Poisson_scale;

		X = &XYZ[Nn*0];
		Y = &XYZ[Nn*1];
		Z = &XYZ[Nn*(d-1)];

		for (eq = 0; eq < Neq; eq++) {
			for (n = 0; n < Nn; n++) {
				if (d == 2)
//					source[eq*Nn+n] = -2.0*PI*PI*sin(PI*X[n])*sin(PI*Y[n]);
					source[eq*Nn+n] = -2.0*pow(Poisson_scale*PI,2.0)*cos(Poisson_scale*PI*X[n])*cos(Poisson_scale*PI*Y[n]);
//					source[eq*Nn+n] = PI*(Y[n]*sin(PI*Y[n])*(2*cos(PI*X[n])-PI*X[n]*sin(PI*X[n]))
//					                     +X[n]*sin(PI*X[n])*(2*cos(PI*Y[n])-PI*Y[n]*sin(PI*Y[n])));
				else if (d == 3)
					source[eq*Nn+n] = -3.0*PI*PI*sin(PI*X[n])*sin(PI*Y[n])*sin(PI*Z[n]);
			}
		}
	} else {
		printf("Error: Unsupported TestCase.\n"), EXIT_MSG;
	}
}
