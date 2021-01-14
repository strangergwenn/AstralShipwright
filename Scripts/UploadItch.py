#!/usr/bin/env python
# -*- coding: utf-8 -*-

#-------------------------------------------------------------------------------
# Upload the game for distribution - make sure to configure project.json
# Usage : UploadItch.py <absolute-output-dir>
# 
# Gwennaël Arbona 2019
#-------------------------------------------------------------------------------

import os
import json
import subprocess


#-------------------------------------------------------------------------------
# Read config files for data
#-------------------------------------------------------------------------------

projectConfigFile = open('../Config/Build.json')
projectConfig = json.load(projectConfigFile)

# Get optional build settings
outputDir =                  str(projectConfig.get('outputDir'))

# Get Itch settings
itchConfig =                 projectConfig["itch"]
itchUser =                   str(itchConfig["user"])
itchProject =                str(itchConfig["project"])
itchBranches =               itchConfig["branches"]
itchDirectories =            itchConfig["directories"]


#-------------------------------------------------------------------------------
# Process arguments for sanity
#-------------------------------------------------------------------------------
	
# Get output directory
if outputDir == 'None':
	if len(sys.argv) == 2:
		outputDir = sys.argv[1]
	else:
		sys.exit('Output directory was neither set in project.json nor passed as command line')


#-------------------------------------------------------------------------------
# Upload all platforms to itch.io
#-------------------------------------------------------------------------------

itchBranchIndex = 0
for branch in itchBranches:

	# Upload
	subprocess.check_call([
		'butler',
		'push',
		os.path.join(outputDir, itchDirectories[itchBranchIndex]),
		itchUser + "/" + itchProject + ":" + branch
	])
	
	itchBranchIndex += 1
