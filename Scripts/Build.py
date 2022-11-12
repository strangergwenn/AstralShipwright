#!/usr/bin/env python
# -*- coding: utf-8 -*-

#-------------------------------------------------------------------------------
# Package the game for distribution
# Usage : Build.py <absolute-output-dir>
#  - Make sure to configure Build.json
# 
# Gwennaël Arbona 2022
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
if 'UE_ROOT' in os.environ:
	engineDir = os.environ['UE_ROOT']
	if not os.path.exists(engineDir):
		sys.exit('Engine directory provided in %UE_ROOT% does not exist')
else:
	sys.exit('Engine directory was set neither in project.json nor UE_ROOT environment variable')

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

# Clean up output path
releaseOutputDir = os.path.join(os.getcwd(), '..', 'Releases', buildVersion)
if os.path.exists(releaseOutputDir):
	shutil.rmtree(releaseOutputDir)


#-------------------------------------------------------------------------------
# Build project
#-------------------------------------------------------------------------------

# Execute build command for each platform
for platform in projectPlatforms:

	cleanPlatformNames = {
		'Win64' : 'Windows',
		'Linux' : 'Linux'
	}
	
	# Clean up output path
	buildOutputDir = os.path.join(outputDir, cleanPlatformNames[platform])
	if os.path.exists(buildOutputDir):
		shutil.rmtree(buildOutputDir)
	releasePlatformDir = os.path.join(releaseOutputDir, cleanPlatformNames[platform])

	# Build project
	subprocess.check_call([

		engineBuildTool,
		
		# SDK
		'-ScriptsForProject="' + projectFile + '"',
		'Turnkey',
		'-command=VerifySdk',
		'-platform=' + platform,
		'-UpdateIfNeeded',
		
		# Executable
		'BuildCookRun',
		'-unrealexe=UnrealEditor-Cmd.exe',

		# Project
		'-project="' + projectFile + '"',
		'-utf8output', '-nop4',

		# Build
		'-nocompile',
		'-clientconfig=' + projectBuildConfiguration,
		'-build',
		'-targetplatform=' + platform,
		cleanOption,
		installedOption,

		# Cook
		'-cook',
		'-skipcookingeditorcontent',
		'-createreleaseversion=' + buildVersion,

		# Package
		'-stage', 
		'-package',
		'-pak',
		'-compressed',
		'-distribution',
		'-archive',
		'-archivedirectory="' + outputDir + '"'
	])

	# Remove individual files that are not wanted in release
	for root, directories, filenames in os.walk(buildOutputDir):
		for filename in filenames:
			
			absoluteFilename = str(os.path.join(root, filename))
			baseChunkName = projectName + '-' + cleanPlatformNames[platform] + '-'
		
			# Wipe generated files that aren't needed
			if re.match('.*\.((pdb)|(debug)|(sym))', filename):
				if 'ThirdParty' in root or not projectKeepPdbs:
					shutil.move(absoluteFilename, releasePlatformDir)
			elif re.match('.*\.exe', filename):
				shutil.copyfile(absoluteFilename, os.path.join(releasePlatformDir, filename))
			elif re.match('Manifest.*\.txt', filename):
				os.remove(absoluteFilename)
				
			# Rename optional chunks
			chunkMatch = re.search('pakchunk([0-9]+)optional.*\.pak', filename)
			if chunkMatch:
				absoluteChunkFilename = str(os.path.join(root, baseChunkName + chunkMatch.group(1) + 'b.pak'))
				shutil.move(absoluteFilename, absoluteChunkFilename)
	
			# Rename normal chunks
			else:
				chunkMatch = re.search('pakchunk([0-9]+).*\.pak', filename)
				if chunkMatch:
					absoluteChunkFilename = str(os.path.join(root, baseChunkName + chunkMatch.group(1) + '.pak'))
					shutil.move(absoluteFilename, absoluteChunkFilename)
	
	# Remove unwanted config directories, engine debug content, unused libraries
	shutil.rmtree(os.path.join(buildOutputDir, projectName, 'Config'), ignore_errors=True)
	shutil.rmtree(os.path.join(buildOutputDir, 'Engine', 'Content'), ignore_errors=True)
	shutil.rmtree(os.path.join(buildOutputDir, 'Engine', 'Binaries', 'ThirdParty', 'NVIDIA'), ignore_errors=True)
	shutil.rmtree(os.path.join(buildOutputDir, 'Engine', 'Binaries', 'ThirdParty', 'PhysX3'), ignore_errors=True)
	
	# Copy Steam appId file
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
