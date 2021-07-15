#!/usr/bin/env python
# -*- coding: utf-8 -*-

#-------------------------------------------------------------------------------
# Package the game for distribution - make sure to configure project.json
# Usage : Build.py <absolute-output-dir>
# 
# Gwennaël Arbona 2019
#-------------------------------------------------------------------------------

import re
import os
import sys
import json
import shutil
import subprocess


#-------------------------------------------------------------------------------
# Read config files for data
#-------------------------------------------------------------------------------

projectConfigFile = open('../Config/Build.json')
projectConfig = json.load(projectConfigFile)

# Get mandatory project settings
projectName =                str(projectConfig['name'])
projectPlatforms =           projectConfig['platforms']
projectBuildConfiguration =  projectConfig['configuration']
projectKeepPdbs =            projectConfig['pdb']

# Get optional build settings
outputDir =                  os.path.abspath(str(projectConfig.get('outputDir')))
engineDir =                  str(projectConfig.get('engineDir'))
sourceEngine =               projectConfig.get('sourceEngine')

projectConfigFile.close()

# Get version tag
gitCommand = ['git', 'describe', '--tags']
try:
	buildVersion = subprocess.check_output(gitCommand).decode('utf-8')
	buildVersion = buildVersion.replace('\n', '');
except:
	sys.exit('Git describe failed')


#-------------------------------------------------------------------------------
# Process arguments for sanity
#-------------------------------------------------------------------------------
	
# Get output directory
if outputDir == 'None':
	if len(sys.argv) == 2:
		outputDir = sys.argv[1]
	else:
		sys.exit('Output directory was neither set in project.json nor passed as command line')

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

# Installed vs built engine
if sourceEngine:
	installedOption = ''
	cleanOption = ''
else:
	installedOption = '-installed'
	cleanOption = ''

# Generate paths
projectFile = os.path.join(os.getcwd(), '..', projectName + '.uproject')
engineBuildTool = os.path.join(engineDir, 'Engine', 'Build', 'BatchFiles', 'RunUAT.bat')


#-------------------------------------------------------------------------------
# Build project
#-------------------------------------------------------------------------------

# Execute build command for each platform
for platform in projectPlatforms:

	# Get platform path
	if platform == 'Linux':
		buildOutputDir = outputDir + "/LinuxNoEditor"
	else:
		buildOutputDir = outputDir + "/WindowsNoEditor"

	# Clean up
	if os.path.exists(buildOutputDir):
		shutil.rmtree(buildOutputDir)

	# Build project
	subprocess.check_call([

		# Executable
		engineBuildTool,
		'BuildCookRun',
		'-ue4exe=UE4Editor-Cmd.exe',

		# Project
		'-project="' + projectFile + '"',
		'-utf8output', '-nop4', 

		# Build
		'-nocompile',
		'-clientconfig=' + projectBuildConfiguration,
		'-build', '-targetplatform=' + platform,
		cleanOption,
		installedOption,

		# Cook
		'-cook',
		'-createreleaseversion=' + buildVersion,

		# Package
		'-stage', 
		'-package',
		'-pak', '-compressed',
		'-distribution',
		'-archive', '-archivedirectory="' + outputDir + '"'
	])

	# Post-processing of generated files
	for root, directories, filenames in os.walk(buildOutputDir):
		for filename in filenames:
			
			absoluteFilename = str(os.path.join(root, filename))
		
			# Wipe generated files that aren't needed
			if re.match('.*\.((pdb)|(debug))', filename):
				if 'ThirdParty' in root or not projectKeepPdbs:
					shutil.move(absoluteFilename, os.path.join(os.getcwd(), '..', 'Releases', buildVersion))
			elif re.match('Manifest.*\.txt', filename):
				os.remove(absoluteFilename)
	
			# Rename chunks
			chunkMatch = re.search('pakchunk([0-9]+).*\.pak', filename)
			if chunkMatch:
				absoluteChunkFilename = str(os.path.join(root, projectName + '-' + platform + '-' + chunkMatch.group(1) + '.pak'))
				os.rename(absoluteFilename, absoluteChunkFilename)
	
	# Remove engine content - only debug stuff
	shutil.rmtree(os.path.join(buildOutputDir, 'Engine', 'Content'))
	
	# Copy Steam Appid file
	buildExecutableDir = os.path.join(buildOutputDir, projectName, 'Binaries', platform)
	shutil.copyfile(os.path.join('..', 'steam_appid.txt'), os.path.join(buildExecutableDir, 'steam_appid.txt'))
	
	# Copy the crash reporter
	crashReportExecutableDir = os.path.join(buildOutputDir, 'Engine', 'Binaries', platform)
	crashReportExecutable = os.path.join('..', 'Mayday')
	os.mkdir(crashReportExecutableDir)
	if platform == 'Linux':
		if os.path.exists(crashReportExecutable):
			shutil.copyfile(crashReportExecutable, os.path.join(crashReportExecutableDir, 'CrashReportClient'))
	else:
		crashReportExecutable = crashReportExecutable + '.exe'
		if os.path.exists(crashReportExecutable):
			shutil.copyfile(crashReportExecutable, os.path.join(crashReportExecutableDir, 'CrashReportClient.exe'))
