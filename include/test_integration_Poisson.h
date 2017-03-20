// Copyright 2017 Philip Zwanenburg
// MIT License (https://github.com/PhilipZwanenburg/DPGSolver/blob/master/LICENSE)

#ifndef DPG__test_integration_Poisson_h__INCLUDED
#define DPG__test_integration_Poisson_h__INCLUDED

#include <stdbool.h>

extern char *get_fNameOut            (char *output_type);
extern void set_PrintName_ConvOrders (char *PrintName, bool *TestTRI);
extern void test_integration_Poisson (int nargc, char **argv);

#endif // DPG__test_integration_Poisson_h__INCLUDED
