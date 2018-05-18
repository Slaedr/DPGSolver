
"""
Module: Basis.py
------------------------------------------

Module that is in charge of creating the NURBS basis functions. 
It will take in the information for the given patch and 
create the B Spline basis, weight function and then ultimately
the NURBS basis (we will not go to that high of Mesh levels so
this function will not need to be that computationally efficient).

"""

import numpy
import matplotlib.pyplot as plt

# Used for floating point comparison
CONST_eps = 1E-14


def N_ip(i,p,t,tVector):

	"""
	The B Spline basis function. This function evaluates the 
	1D basis function at a given point based on the order and 
	knot vector. The evaluation is done using the recursive definition 
	of the B spline basis. 

	NOTE : 
		- For the B Spline basis, any 0/0 term is defined to be 0. 
			Since floating point values are used, compare the numerator
			and denominator to an epsilon parameter.

	:param i: The ith basis function (B Spline) to use for the evaluation
	:param p: The order of the basis functions
	:param t: The value at which to evaluate the basis function at (will be
		in the domain of the knot vector).
	:param tVector: The knot vector (one dimension) for the basis.

	:return : Float for the value of the given basis function at the given point.
	"""
	
	# Handle the case where t is close to the edges
	if (abs(t-tVector[0]) < CONST_eps):
		t = tVector[0] + CONST_eps
	elif (abs(t-tVector[-1]) < CONST_eps):
		t = tVector[-1] - CONST_eps

	# Base case:
	if p == 0:
		if t < tVector[i+1] and t >= tVector[i]:
			return 1.
		else:
			return 0.
	
	else:
		# Recursive case:
		
		t_i = tVector[i]
		t_iPlus1 = tVector[i+1]
		t_iPlusP = tVector[i+p]
		t_iPlusPPlus1 = tVector[i+p+1]
			
		#	The first term
		num1 = (t - t_i) * N_ip(i, p-1, t, tVector)
		denom1 = t_iPlusP - t_i

		if abs(num1) < CONST_eps and abs(denom1) < CONST_eps:
			term1 = 0.
		else:
			term1 = num1/denom1

		#	The second term
		num2 = (t_iPlusPPlus1 - t) * N_ip(i+1, p-1, t, tVector)
		denom2 = t_iPlusPPlus1 - t_iPlus1
		if abs(num2) < CONST_eps and abs(denom2) < CONST_eps:
			term2 = 0.
		else:
			term2 = num2/denom2

		return term1 + term2


def weight_function(BSplineBasis, Weights, xi, eta):

	"""
	Compute the 2D weight function for the given mesh. To get the
	NURBS basis, each B Spline basis is divided by the weight function, thus
	giving the rational nature for the basis. 

	The weight function will be given by the summation of each basis function
	multiplied by its corresponding weight. The following is an example of 
	the weight function for a 2D case with a total of N_Basis basis functions

		W(xi, eta) = \sum_{i = 1 to N_Basis} N_ip(xi,eta)*W_i
	
	:param BSplineBasis: The matrix of B Spline basis functions (2D mesh case).
	:param Weights : The matrix of weights for each basis function / control
		point for the mesh.
	:param xi: The xi value (float) at which to evaluate the basis at
	:param eta: The eta value (float) at which to evaluate the basis at

	:return : The value of the weight function at the given point on 
		the knot parametric domain.
	"""

	value = 0

	# Matrix, so each inner array should be of same size.
	# The Weights matrix should also be of the same dimension
	numI = len(BSplineBasis)
	numJ = len(BSplineBasis[0])

	for i in range(numI):
		for j in range(numJ):
			value = value + BSplineBasis[i][j](xi,eta)*Weights[i][j]
			
	return value

def get_BSpline_basis_functions_1D(P, xiVector):

	"""
	Get the B spline basis functions and return them as a list of 
	lambda expressions. 

	There are n = (len(xiVector) - P - 1) total basis functions

	:param P: The order of the basis functions
	:param xiVector: The knot vector

	:return : The lambda expressions for the basis (in a list)
	"""

	n = len(xiVector) - P - 1
	
	if n <= 0:
		raise ValueError("Insufficient number of basis functions")

	N_Basis = []
	for i in range(n):
		N_ip_xi = lambda xi, i=i: N_ip(i, P, xi, xiVector)
		N_Basis.append(N_ip_xi)

	return N_Basis
	
def weight_function_1D(BSplineBasis, Weights, xi):

	"""
	Compute the weight function (1D) at a given point 
	xi on the parametric knot domain

	:param BSplineBasis: The list of B Spline Basis functions
	:param Weights: The list of weights
	:param xi: The value to evaluate the weight function

	:return : The value of the weight function at the given xi point
	"""

	value = 0

	for i in range(len(BSplineBasis)):
		value = value + BSplineBasis[i](xi)*Weights[i]
			
	return value


def get_NURBS_basis_functions_1D(P, xiVector, wVector):

	"""
	Get the NURBS basis functions and return them as a list of 
	lambda expressions. 

	There are n = (len(xiVector) - P - 1) total basis functions

	:param P: The order of the basis functions
	:param xiVector: The knot vector
	:param wVector: The vector of weights

	:return : The lambda expressions for the basis (in a list)
	"""

	BSplineBasis = get_BSpline_basis_functions_1D(P, xiVector)

	# Compute the Weight function
	w_func = lambda xi, BSplineBasis = BSplineBasis: weight_function_1D(BSplineBasis, wVector, xi)

	NURBS_Basis = []
	for i in range(len(BSplineBasis)):
		R = lambda xi, i=i: \
				(wVector[i] * BSplineBasis[i](xi))/w_func(xi)
		NURBS_Basis.append(R)

	return NURBS_Basis


def get_NURBS_basis_functions(ControlPointsAndWeights, P, Q, xiVector, etaVector):
   
	"""
	Get the NURBS basis functions by using the control points,
	knot vectors and weight values for the control points. 
	
	:param ControlPointsAndWeights: The matrix of control points and their weights.
		i,j index of the matrix points to a list of the form [x,y,weight]
	:param P: The order of the mesh in the xi direction
	:param Q: The order of the mesh in the eta direction
	:param xiVector: The knot vector for the xi direction
	:param etaVector: The knot vector for the eta direction

	:return : The lambda expressions for the NURBS basis functions for
		each control point. A list of lists of basis functions will be 
		returned in the 2D case. The ordering of the basis (which i,j index)
		is consistent with the ordering of their respective control points
	"""

	# Get the number of control points in the i (xi) and j (eta) directions 
	numI = len(ControlPointsAndWeights)
	numJ = len(ControlPointsAndWeights[0])

	# Create the empty list to hold the NURBS basis functions (R_ij_pq) and
	# the B spline basis functions (N_ij_pq), which are needed to find the 
	# NURBS functions
	N_ij_pq = []
	R_ij_pq = []
	for i in range(numI):
		colArray = []
		for j in range(numJ):
			colArray.append(None)
		N_ij_pq.append(colArray)
		R_ij_pq.append(colArray[:])  # Use a copy of the empty array by value

	# Compute the B Spline basis functions (N_ip)
	for i in range(numI):
		N_ip_xi = lambda xi, i=i: N_ip(i, P, xi, xiVector)     
		for j in range(numJ):
			N_jq_eta = lambda eta, j=j: N_ip(j, Q, eta, etaVector)  
			N_ij_pq_xieta = lambda xi,eta, N_ip_xi=N_ip_xi, N_jq_eta=N_jq_eta: \
				(N_ip_xi(xi)*N_jq_eta(eta))
			N_ij_pq[i][j] = N_ij_pq_xieta  # Save the lambda expression for the i,j basis
	
	# Store all the weights in their own seperate matrix
	weights = []
	for i in range(numI):
		colArray = []
		for j in range(numJ):
			colArray.append(ControlPointsAndWeights[i][j][2])
		weights.append(colArray)

	# Compute the Weight function
	w_func = lambda xi, eta, N_ip = N_ij_pq: weight_function(N_ip, weights, xi, eta)

	# Set the NURBS Basis functions using the weight function, the B Spline basis,
	# and the weights.
	for i in range(numI):
		for j in range(numJ):
			R = lambda xi, eta, i=i, j=j: \
				(weights[i][j] * N_ij_pq[i][j](xi,eta))/w_func(xi,eta)
			R_ij_pq[i][j] = R

	return R_ij_pq

def plot_basis_1D(basis_function_list, xi_min, xi_max):

	"""
	Plot the basis functions on the domain xi_min to 
	xi_max

	:param basis_function_list: The list of basis functions (lambda expressions)
	:param xi_min: The lower limit of the domain to plot
	:param xi_max: The upper limit of the domain to plot

	:return : -
	"""

	num_plot_pts = 50
	plot_pts = numpy.linspace(xi_min, xi_max, num_plot_pts)

	for N_b in basis_function_list:

		y_vals = []

		for pt in plot_pts:
			y_vals.append(N_b(pt))

		plt.plot(plot_pts, y_vals)

	plt.show(block=True)

def test_basis():
	
	xi_vector = [-2,-2,-2,-1,0,1,2,2,2]
	w_vector = [0.5, 1.75, 0.8, 1.8, 1.75]
	P = 3

	#basis_funcs = get_BSpline_basis_functions_1D(P, xi_vector)
	basis_funcs = get_NURBS_basis_functions_1D(P, xi_vector, w_vector)

	plot_basis_1D(basis_funcs, xi_vector[0], xi_vector[-1])

if __name__ == "__main__":
	test_basis()




