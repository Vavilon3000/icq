from __future__ import print_function
import os
import fileinput
import json
import sys
import md5

ICQ_VERSION_MAJOR = '10'
ICQ_VERSION_MINOR = '0'

def get_str_version_number(build_number):	
	return ".".join((ICQ_VERSION_MAJOR, ICQ_VERSION_MINOR, build_number))
	
def update_version(build_number):
	prod_version_str_macro = "#define VERSION_INFO_STR"
	file_version_str_macro = "#define VER_FILEVERSION_STR"
	file_version_macro = "#define VER_FILEVERSION"

	STR_VERSION = get_str_version_number(build_number)
	VERSION = STR_VERSION.replace('.', ',')

	version_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), "../common.shared/version_info_constants.h") 

	for line in fileinput.input(version_path, inplace=True):
		if prod_version_str_macro in line:
			line = prod_version_str_macro + ' "'+STR_VERSION+'"\n'
		elif file_version_str_macro in line:
			line = file_version_str_macro + ' "'+STR_VERSION+'"\n'
		elif file_version_macro in line:
			line = file_version_macro + ' '+VERSION+'\n'
		print(line, end='')

if len(sys.argv) < 2:
	print("Not enough arguments")

build_number = os.getenv('BUILD_NUMBER', '1999')

if sys.argv[1] == "--update":
	update_version(build_number)

elif sys.argv[1] == "--make-json":
	version = {}
	version["major"] = int(ICQ_VERSION_MAJOR)
	version["minor"] = int(ICQ_VERSION_MINOR)
	version["buildnumber"] = int(build_number)
	info = {}
	info["version"] = version
	info["file"] = "updater"
	with open(os.path.join(os.getcwd(), sys.argv[2]), 'rb') as f:
		buf = f.read()
	info["md5"] = md5.new(buf).hexdigest()
	data = {}
	data["info"] = info

	json_data = json.dumps(data)
	f = open(os.path.join(os.getcwd(), sys.argv[3]) + '/version.js', 'w')
	f.write(json_data)
	f.close()



	




