#!/usr/bin/env python
# -*- coding: utf-8 -*-

#-------------------------------------------------------------------------------
# Launch an instance of the game as server - make sure to configure project.json
#-------------------------------------------------------------------------------

import os
import sys
import json
import subprocess


#-------------------------------------------------------------------------------
# Process host argument
#-------------------------------------------------------------------------------

hostArg = ''
if len(sys.argv) == 2:
	if sys.argv[1] == 'host':
		hostArg = '-host'


#-------------------------------------------------------------------------------
# Read config files for data
#-------------------------------------------------------------------------------

# Load config
projectConfigFile = open('../Config/Build.json')
projectConfig = json.load(projectConfigFile)

# Get mandatory project settings
projectName =                str(projectConfig['name'])

# Get optional build settings
engineDir =                  str(projectConfig.get('engineDir'))

projectConfigFile.close()

# Get engine directory
if engineDir != 'None':
	if not os.path.exists(engineDir):
		sys.exit('Engine directory provided in project.json does not exist')
if 'UE4_ROOT' in os.environ:
	engineDir = os.environ['UE4_ROOT']
	if not os.path.exists(engineDir):
		sys.exit('Engine directory provided in %UE4_ROOT% does not exist')
else:
	sys.exit('Engine directory was set neither in project.json nor UE4_ROOT environment variable')

# Generate paths
projectFile = os.path.join(os.getcwd(), '..', projectName + '.uproject')
engineExecutable = os.path.join(engineDir, 'Engine', 'Binaries', 'Win64', 'UE4Editor.exe')


#-------------------------------------------------------------------------------
# Launch
#-------------------------------------------------------------------------------

subprocess.Popen([
	engineExecutable,
	projectFile,
	'-skipcompile',
	'-game',
	'-ResX=1920',
	'-ResY=1040',
	'-WinX=1920',
	'-WinY=30',
	hostArg
])
