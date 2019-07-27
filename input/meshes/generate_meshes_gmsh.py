import sys
import subprocess
import shlex
import re

class Gmsh_call:
	""" Container for gmsh call related information. """
	def __init__ (self):
		self.dim      = '' ###< The dimension.
		self.in_dir   = '' ###< The full path to the ROOT input directory.
		self.in_name  = '' ###< The name of the input file.
		self.out_name = '' ###< The name of the output file.
		self.args     = '' ###< The arguments to be passed to gmsh

	def set_input (self,project_src_dir,mesh_name):
		self.in_dir  = project_src_dir+"/input/meshes/"
		self.in_name = self.in_dir

		supported_geometries = ["n-cube",
		                        "n-cylinder_hollow_section",
		                        "n-cylinder_hollow",
		                       ]

		geometry = mesh_name.split('/')[0]
		if (geometry not in supported_geometries):
			print("\n\nThe ",geometry," is not currently supported.\n\n\n")
			EXIT

		geo_name = re.search(r"(__)([\w]+)(__)",mesh_name).group(2)

		self.in_name += geometry+'/'+geo_name+".geo"

		self.dim = re.search(r"(^[\D]*)(\d)([\D]*$)",geo_name).group(2)

	def set_output (self,mesh_name_full):
		self.out_name = mesh_name_full

	def set_args (self,mesh_name):
		self.args  = self.in_name
		self.args += " -" + self.dim
		self.args += set_gmsh_setnumbers(self.in_dir,mesh_name)
		self.args += " -o " + self.out_name

	def call_function (self,mesh_name_full):
		out_dir = re.search(r"/(([\w-]+/)*)",mesh_name_full).group(0)
		subprocess.call(shlex.split("mkdir -p " + out_dir))

		subprocess.call(shlex.split("gmsh -format msh2 " + self.args))

def set_gmsh_setnumbers (input_dir,mesh_name):
	""" Set the -setnumber inputs to be passed to gmsh. """
	gmsh_setnumbers = ''

	# Required parameters

	mesh_level = re.search(r"(^.*_ml)(\d+)(.*$)",mesh_name).group(2)
	gmsh_setnumbers += " -setnumber mesh_level " + mesh_level

	gmsh_setnumbers += " -setnumber pde_name "
	var_names = ["advection","diffusion","euler","navier_stokes","burgers_inviscid",]
	gmsh_setnumbers += get_gmsh_number_from_mesh_name(mesh_name,var_names,input_dir,0)

	gmsh_setnumbers += " -setnumber mesh_domain "
	var_names = ["straight","blended","parametric"]
	gmsh_setnumbers += get_gmsh_number_from_mesh_name(mesh_name,var_names,input_dir,0)

	gmsh_setnumbers += " -setnumber mesh_type "
	var_names = ["line","tri","quad","tet","hex","wedge","pyr","mixed"]
	gmsh_setnumbers += get_gmsh_number_from_mesh_name(mesh_name,var_names,input_dir,1)


	# Additional spec parameters
	gmsh_setnumbers += " -setnumber pde_spec "
	var_names = ["steady/supersonic_vortex",
	             "periodic/periodic_vortex",
	             "steady/taylor_couette",
	             "steady/joukowski",
	             "steady/default",
	             "steady/free_stream",
	             "steady/vortex",
	             "steady/gaussian_bump",
	             "default_steady",
	             "demkowicz_dpg_ii",
	             "unsteady/hyperbolic_tan",
	             "periodic/trigonometric",
	            ]
	gmsh_setnumbers += get_gmsh_number_from_mesh_name(mesh_name,var_names,input_dir,0)

	gmsh_setnumbers += " -setnumber geom_adv "
	if (mesh_name.find("/xyz_l/") != -1):
		gmsh_setnumbers += get_gmsh_number("geom_adv_xyzl",input_dir,0)
	elif (mesh_name.find("/xy_l/") != -1):
		gmsh_setnumbers += get_gmsh_number("geom_adv_xyl",input_dir,0)
	elif (mesh_name.find("/x_l/") != -1):
		gmsh_setnumbers += get_gmsh_number("geom_adv_xl",input_dir,0)
	elif (mesh_name.find("/x_lr/") != -1):
		gmsh_setnumbers += get_gmsh_number("geom_adv_xlr",input_dir,0)
	elif (mesh_name.find("/y_l/") != -1):
		gmsh_setnumbers += get_gmsh_number("geom_adv_yl",input_dir,0)
	elif (mesh_name.find("/xl_yr/") != -1):
		gmsh_setnumbers += get_gmsh_number("geom_adv_xl_yr",input_dir,0)
	elif (mesh_name.find("/periodic/") != -1):
		gmsh_setnumbers += get_gmsh_number("geom_adv_periodic",input_dir,0)
	elif (mesh_name.find("/upwind/") != -1):
		gmsh_setnumbers += get_gmsh_number("geom_adv_upwind",input_dir,0)
	elif (mesh_name.find("/x_lr_internal/") != -1):
		gmsh_setnumbers += get_gmsh_number("geom_adv_internal",input_dir,0)
	else:
		gmsh_setnumbers += get_gmsh_number("gmsh_dummy",input_dir,0)

	gmsh_setnumbers += " -setnumber geom_bc "
	if (mesh_name.find("/adiabatic_o/") != -1):
		gmsh_setnumbers += get_gmsh_number("geom_bc_adiabatic_o",input_dir,0)
	elif (mesh_name.find("/diabatic_o/") != -1):
		gmsh_setnumbers += get_gmsh_number("geom_bc_diabatic_o",input_dir,0)
	elif (mesh_name.find("/slipwall/") != -1):
		gmsh_setnumbers += get_gmsh_number("geom_bc_slipwall",input_dir,0)
	else:
		gmsh_setnumbers += get_gmsh_number("gmsh_dummy",input_dir,0)

	gmsh_setnumbers += " -setnumber geom_ar "
	geom_spec_ar = re.findall("geom_ar_\d+\-*\d*",mesh_name)
	if (len(geom_spec_ar) >= 1):
		assert (len(geom_spec_ar) == 1),"More than one occurence of \"geom_ar_\\d+\" found."
		geom_ar = (geom_spec_ar[0].replace("geom_ar_","")).replace("-",".")
		gmsh_setnumbers += geom_ar
	else:
		gmsh_setnumbers += get_gmsh_number("gmsh_dummy",input_dir,0)

	gmsh_setnumbers += " -setnumber geom_unaligned "
	if (mesh_name.find("/unaligned/") != -1):
		gmsh_setnumbers += '1'
	else:
		gmsh_setnumbers += '0'

	gmsh_setnumbers += " -setnumber geom_conformal "
	if (mesh_name.find("/conformal_half/") != -1):
		gmsh_setnumbers += get_gmsh_number("geom_conformal_half",input_dir,0)
	elif (mesh_name.find("/conformal_full/") != -1):
		gmsh_setnumbers += get_gmsh_number("geom_conformal_full",input_dir,0)
	else:
		gmsh_setnumbers += get_gmsh_number("gmsh_dummy",input_dir,0)

	return gmsh_setnumbers


def get_gmsh_number_from_mesh_name (mesh_name,var_names,input_dir,with_underscore):
	""" Get the number associated with the variable name found in the mesh_name as specified in the parameters.geo
	    file. """

	param_file_name = input_dir+"/parameters.geo"

	var_name = "gmsh_dummy"
	found = False
	for target in var_names:
		target_name = target
		if (with_underscore):
			target_name = '_'+target_name+'_'

		if (mesh_name.find(target_name) != -1):
			var_name = target.replace('/','_')
			found = True
			break
	assert found, "Did not find the variable name ("+mesh_name+")in the mesh_name."

	return get_gmsh_number(var_name,input_dir,with_underscore)


def get_gmsh_number (var_name,input_dir,with_underscore):
	""" Get the number associated with the single variable name as specified in the parameters.geo file. """

	param_file_name = input_dir+"/parameters.geo"

	with open(param_file_name) as f:
		for line in f:
			if (var_name.upper() in line):
				return line.split()[2][:-1]

	print("\n\nDid not find a value for "+var_name.upper()+" in "+param_file_name+'\n')
	EXIT


if __name__ == "__main__":
	""" Generate the gmsh mesh passed as a command line argument using the appropriate .geo file. """


	if (len(sys.argv) > 3):
		print("Only a single mesh_name should be input at a time.\n")
		EXIT

	project_src_dir = sys.argv[1]
	mesh_name_full  = sys.argv[2]

	mesh_name = re.sub(".*meshes/","",mesh_name_full)

	gmsh_call = Gmsh_call()
	gmsh_call.set_input(project_src_dir,mesh_name)
	gmsh_call.set_output(mesh_name_full)
	gmsh_call.set_args(mesh_name)
	gmsh_call.call_function(mesh_name_full)
