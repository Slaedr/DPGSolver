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
/// \file

#include "element.h"

#include <string.h>

#include "macros.h"
#include "definitions_elements.h"
#include "definitions_intrusive.h"

#include "multiarray.h"
#include "const_cast.h"

// Static function declarations ************************************************************************************* //

/** \brief Constructor for an individual \ref Element.
 *  \return Standard. */
static struct Element* constructor_Element
	(const int elem_type ///< The element type (e.g. LINE, TRI, ...)
	);

// Interface functions ********************************************************************************************** //

struct const_Intrusive_List* constructor_Elements (const int d)
{
	struct Intrusive_List* elements = constructor_empty_IL(IL_ELEMENT);

	push_back_IL(elements,(struct Intrusive_Link*) constructor_Element(LINE));

	if (d >= 2) {
		push_back_IL(elements,(struct Intrusive_Link*) constructor_Element(TRI));
		push_back_IL(elements,(struct Intrusive_Link*) constructor_Element(QUAD));
	}

	if (d >= 3) {
		push_back_IL(elements,(struct Intrusive_Link*) constructor_Element(TET));
		push_back_IL(elements,(struct Intrusive_Link*) constructor_Element(HEX));
		push_back_IL(elements,(struct Intrusive_Link*) constructor_Element(WEDGE));
		push_back_IL(elements,(struct Intrusive_Link*) constructor_Element(PYR));
	}

	return (struct const_Intrusive_List*) elements;
}

void destructor_Elements (struct Intrusive_List* elements)
{
	for (const struct Intrusive_Link* curr = elements->first; curr; ) {
		struct Intrusive_Link* next = curr->next;
		destructor_Element((struct Element*) curr);
		curr = next;
	}
	destructor_IL(elements);
}

void destructor_const_Elements (const struct const_Intrusive_List* elements)
{
	destructor_Elements((struct Intrusive_List*)elements);
}

void destructor_Element (struct Element* element)
{
	destructor_const_Multiarray_Vector_i(element->f_ve);
}

void const_cast_const_Element (const struct const_Element*const* dest, const struct const_Element*const src)
{
	*(struct const_Element**) dest = (struct const_Element*) src;
}

struct const_Element* get_element_by_type (const struct const_Intrusive_List*const elements, const int type)
{
	for (const struct Intrusive_Link* curr = elements->first; curr; curr = curr->next) {
		struct const_Element* element = (struct const_Element*) curr;
		if (element->type == type)
			return element;
	}
	printf("Could not find the element of type: %d.\n",type);
	EXIT_UNSUPPORTED;
}

struct const_Element* get_element_by_face (const struct const_Element*const element, const int lf)
{
	int type_to_find = -1;
	switch (element->type) {
	case LINE:
		type_to_find = POINT;
		break;
	case TRI: case QUAD:
		type_to_find = LINE;
		break;
	case TET:
		type_to_find = TRI;
		break;
	case HEX:
		type_to_find = QUAD;
		break;
	case WEDGE:
		if (lf < 3)
			type_to_find = QUAD;
		else
			type_to_find = TRI;
		break;
	case PYR:
		if (lf < 4)
			type_to_find = TRI;
		else
			type_to_find = QUAD;
		break;
	default:
		EXIT_UNSUPPORTED;
		break;
	}

	for (const struct Intrusive_Link* curr = (const struct Intrusive_Link*) element; curr; curr = curr->prev) {
		struct const_Element* element_curr = (struct const_Element*) curr;
		if (element_curr->type == type_to_find)
			return element_curr;
	}
	EXIT_ERROR("Did not find the pointer to the face element");
}

bool wedges_present (const struct const_Intrusive_List*const elements)
{
	for (const struct Intrusive_Link* curr = elements->first; curr; curr = curr->next) {
		struct const_Element* element = (struct const_Element*) curr;
		if (element->type == WEDGE)
			return true;
	}
	return false;
}

const int compute_elem_type_sub_ce (const int e_type, const char ce, const int ind_ce)
{
	switch (ce) {
	case 'v':
		switch (e_type) {
		case LINE: // fallthrough
		case TRI:  // fallthrough
		case QUAD: // fallthrough
		case TET:  // fallthrough
		case HEX:  // fallthrough
		case WEDGE:
			return e_type;
		case PYR:
			switch (ind_ce) {
			case 0: // standard pyr
			case 1: case 2:  case 3: case 4: // 4 base pyr sub-elements.
			case 9: case 10: // 2 top pyr sub-elements.
				return PYR;
				break;
			case 5: case 6: case 7: case 8: // 4 tet sub-elements.
				return TET;
				break;
			default:
				EXIT_ERROR("Unsupported: %d\n",ind_ce);
				break;
			}
		default:
			EXIT_ERROR("Unsupported: %d\n",e_type);
			break;
		}
		break;
	case 'f':
		switch (e_type) {
		case LINE:
			return POINT;
			break;
		case TRI: // fallthrough
		case QUAD:
			return LINE;
			break;
		case TET:
			return TRI;
			break;
		case HEX:
			return QUAD;
			break;
		case WEDGE:
			switch (ind_ce) {
			case 0:  case 1:  case 2:           // 3 quad faces of wedge reference element.
			case 5:  case 6:  case 7:  case 8:  // 4 quad refined face 0 sub-faces.
			case 9:  case 10: case 11: case 12: // 4 quad refined face 1 sub-faces.
			case 13: case 14: case 15: case 16: // 4 quad refined face 2 sub-faces.
				return QUAD;
				break;
			case 3: case 4:                     // 2 tri faces of wedge reference element.
			case 17: case 18: case 19: case 20: // 4 tri refined face 3 sub-faces.
			case 21: case 22: case 23: case 24: // 4 tri refined face 4 sub-faces.
				return TRI;
				break;
			default:
				EXIT_ERROR("Unsupported: %d\n",ind_ce);
				break;
			return e_type;
		case PYR:
			switch (ind_ce) {
			case 0:  case 1:  case 2:  case 3:  // 3 tri faces of wedge reference element.
			case 5:  case 6:  case 7:  case 8:  // 4 tri refined face 0 sub-faces.
			case 9:  case 10: case 11: case 12: // 4 tri refined face 1 sub-faces.
			case 13: case 14: case 15: case 16: // 4 tri refined face 2 sub-faces.
			case 17: case 18: case 19: case 20: // 4 tri refined face 3 sub-faces.
				return TRI;
				break;
			case 4:                             // 2 quad faces of wedge reference element.
			case 21: case 22: case 23: case 24: // 4 quad refined face 4 sub-faces.
				return QUAD;
				break;
			default:
				EXIT_ERROR("Unsupported: %d\n",ind_ce);
				break;
			}
		default:
			EXIT_ERROR("Unsupported: %d\n",e_type);
			break;
		}
		break;
	default:
		EXIT_ERROR("Unsupported: %c\n",ce);
		break;
	}
}

// Static functions ************************************************************************************************* //
// Level 0 ********************************************************************************************************** //

///	\brief Container for local element-related information.
struct Elem_info {
	int s_type,  ///< Defined in \ref Element.
	    d,       ///< Defined in \ref Element.
	    n_ve,    ///< Defined in \ref Element.
	    n_e,     ///< Defined in \ref Element.
	    n_f,     ///< Defined in \ref Element.
	    *n_f_ve, ///< The number of vertices on each face.
	    *f_ve;   ///< Defined in \ref Element.

	int n_ref_max,   ///< Defined in \ref Element.
	    n_ref_f_max; ///< Defined in \ref Element.
};

static struct Element* constructor_Element (const int elem_type)
{
	// Note the use of the compound literals for the initialization of the local variables.
	struct Elem_info e_info;
	switch (elem_type) {
	case LINE:
		e_info.s_type = ST_TP;
		e_info.d      = 1;
		e_info.n_ve   = 2;
		e_info.n_e    = 2;
		e_info.n_f    = 2;
		e_info.n_f_ve = (int[]) {1, 1,};
		e_info.f_ve   = (int[]) {0, 1,};

		e_info.n_ref_max   = 3;
		e_info.n_ref_f_max = 1;
		break;
	case TRI:
		e_info.s_type = ST_SI;
		e_info.d      = 2;
		e_info.n_ve   = 3;
		e_info.n_e    = 3;
		e_info.n_f    = 3;
		e_info.n_f_ve = (int[]) {2, 2, 2,};
		e_info.f_ve   = (int[]) {1,2, 0,2, 0,1,};

		e_info.n_ref_max   = 5;
		e_info.n_ref_f_max = 3;
		break;
	case QUAD:
		e_info.s_type = ST_TP;
		e_info.d      = 2;
		e_info.n_ve   = 4;
		e_info.n_e    = 4;
		e_info.n_f    = 4;
		e_info.n_f_ve = (int[]) {2, 2, 2, 2,};
		e_info.f_ve   = (int[]) {0,2, 1,3, 0,1, 2,3};

		e_info.n_ref_max   = 5;
		e_info.n_ref_f_max = 3;
		break;
	case TET:
		EXIT_ADD_SUPPORT;
		break;
	case HEX:
		e_info.s_type = ST_TP;
		e_info.d      = 3;
		e_info.n_ve   = 8;
		e_info.n_e    = 12;
		e_info.n_f    = 6;
		e_info.n_f_ve = (int[]) {4, 4, 4, 4, 4, 4,};
		e_info.f_ve   = (int[]) {0,2,4,6, 1,3,5,7, 0,1,4,5, 2,3,6,7, 0,1,2,3, 4,5,6,7};

		e_info.n_ref_max   = 9;
		e_info.n_ref_f_max = 5;
		break;
	case WEDGE:
		EXIT_ADD_SUPPORT;
		break;
	case PYR:
		EXIT_ADD_SUPPORT;
		break;
	default:
		EXIT_UNSUPPORTED;
		break;
	}

	const ptrdiff_t n_f = e_info.n_f;
	struct Multiarray_Vector_i* f_ve = constructor_copy_Multiarray_Vector_i_i(e_info.f_ve,e_info.n_f_ve,1,&n_f);

	struct Element* element = calloc(1,sizeof *element); // returned

	const_cast_i(&element->type,elem_type);
	const_cast_i(&element->s_type,e_info.s_type);
	const_cast_i(&element->d,e_info.d);
	const_cast_i(&element->n_ve,e_info.n_ve);
	const_cast_i(&element->n_e,e_info.n_e);
	const_cast_i(&element->n_f,e_info.n_f);
	const_cast_i(&element->n_ref_max,e_info.n_ref_max);
	const_cast_i(&element->n_ref_f_max,e_info.n_ref_f_max);
	const_constructor_move_Multiarray_Vector_i(&element->f_ve,f_ve); // destructed

	return element;
}