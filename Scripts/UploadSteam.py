#!/usr/bin/env python
# -*- coding: utf-8 -*-

#-------------------------------------------------------------------------------
# Upload a game for distribution on Steam
#
# Usage : UploadSteam.py <absolute-output-dir>
#  - Make sure to configure STEAM_USER in environment with your Steam username
#  - Make sure to configure STEAM_BUILDER in environment with your Steam SDK 'builder' folder
#  - Make sure to configure Build.json
# 
# Gwennaël Arbona 2021
#-------------------------------------------------------------------------------

import os
import json
import subprocess


#-------------------------------------------------------------------------------
# Read config files for data
#-------------------------------------------------------------------------------

configDir = '../Config/'
projectConfigFile = open(os.path.join(configDir, 'Build.json'))
projectConfig = json.load(projectConfigFile)

# Get optional build settings
outputDir =                  str(projectConfig.get('outputDir'))

# Get Steam settings
steamConfig =                projectConfig["steam"]
steamApps =                  steamConfig["apps"]


#-------------------------------------------------------------------------------
# Process arguments for sanity
#-------------------------------------------------------------------------------
	
# Get output directory
if outputDir == 'None':
	if len(sys.argv) == 2:
		outputDir = sys.argv[1]
	else:
		sys.exit('Output directory was neither set in Build.json nor passed as command line')
		
# Get user name
if 'STEAM_USER' in os.environ:
	steamUser = os.environ['STEAM_USER']
else:
	sys.exit('Steam user was not provided in the STEAM_USER environment variable')
		
# Get builder directory
if 'STEAM_BUILDER' in os.environ:
	steamBuilder = os.environ['STEAM_BUILDER']
else:
	sys.exit('Steam SDK builder directory was not provided in the STEAM_BUILDER environment variable')

# Define the Steam command
if steamBuilder.endswith('builder'):
	steamCommand = 'SteamCmd.exe'
else:
	steamCommand = 'steamcmd.sh'


#-------------------------------------------------------------------------------
# Upload all platforms to Steam
#-------------------------------------------------------------------------------

steamAppIndex = 0
for app in steamApps:

	# Upload
	subprocess.check_call([
		os.path.join(steamBuilder, steamCommand),
		'+login', steamUser,
		'+run_app_build', os.path.realpath(os.path.join(configDir, steamApps[steamAppIndex])),
		'+quit'
	],
	cwd = steamBuilder)

	steamAppIndex += 1
