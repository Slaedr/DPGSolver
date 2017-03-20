// Copyright 2017 Philip Zwanenburg
// MIT License (https://github.com/PhilipZwanenburg/DPGSolver/blob/master/LICENSE)

#include "test_support.h"

#include <stdio.h>

#include "Test.h"

void test_print(const unsigned int pass)
{
	/*
	 *	Purpose:
	 *		Print pass/failure message for testing functions.
	 *
	 *	Comments:
	 *
	 *	Notation:
	 *
	 *	References:
	 */

	TestDB.Ntest++;

	if (pass)
		printf("Pass\n");
	else
		printf("Fail --- Fail --- Fail\n");
}

void test_print2(const unsigned int pass, const char *string)
{
	/*
	 *	Purpose:
	 *		Print pass/failure message for testing functions.
	 *
	 *	Comments:
	 *
	 *	Notation:
	 *
	 *	References:
	 */

	TestDB.Ntest++;

	printf("%-60s",string);
	if (pass)
		printf("Pass\n");
	else
		printf("Fail --- Fail --- Fail\n");
}
