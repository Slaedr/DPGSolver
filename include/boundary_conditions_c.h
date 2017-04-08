// Copyright 2017 Philip Zwanenburg
// MIT License (https://github.com/PhilipZwanenburg/DPGSolver/blob/master/LICENSE)

#ifndef DPG__boundary_conditions_c_h__INCLUDED
#define DPG__boundary_conditions_c_h__INCLUDED

#include <complex.h>


extern void boundary_Riemann_c           (const unsigned int Nn, const unsigned int Nel, const double *const XYZ,
                                          const double complex *const WL, double complex *const WOut,
                                          double complex *const WB, const double *const nL, const unsigned int d);
extern void boundary_SlipWall_c          (const unsigned int Nn, const unsigned int Nel, const double complex *const WL,
                                          double complex *const WB, const double *const nL, const unsigned int d);
extern void boundary_BackPressure_c      (const unsigned int Nn, const unsigned int Nel, const double complex *const WL,
                                          double complex *const WB, const double *const nL, const unsigned int d,
                                          const unsigned int Neq);
extern void boundary_Total_TP_c          (const unsigned int Nn, const unsigned int Nel, const double *const XYZ,
                                          const double complex *const WL, double complex *const WB,
                                          const double *const nL, const unsigned int d, const unsigned int Nvar);
extern void boundary_SupersonicInflow_c  (const unsigned int Nn, const unsigned int Nel, const double *const XYZ,
                                          const double complex *const WL, double complex *const WB,
                                          const double *const nL, const unsigned int d, const unsigned int Nvar);
extern void boundary_SupersonicOutflow_c (const unsigned int Nn, const unsigned int Nel, const double *const XYZ,
                                          const double complex *const WL, double complex *const WB,
                                          const double *const nL, const unsigned int d, const unsigned int Nvar);

#endif // DPG__boundary_conditions_c_h__INCLUDED
