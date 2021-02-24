#!/usr/bin/env python
# -*- coding: utf-8 -*-

#-------------------------------------------------------------------------------
# Format the game sources using clang-format
# Usage : Format.py
# 
# Gwennaël Arbona 2021
#-------------------------------------------------------------------------------

import os
import glob
import json
import subprocess

#-------------------------------------------------------------------------------
# Read config files for data
#-------------------------------------------------------------------------------

projectConfigFile = open('../Config/Build.json')
projectConfig = json.load(projectConfigFile)
projectName =                str(projectConfig['name'])

sourceDir = os.path.join('..', 'Source', projectName)
scriptDir = os.getcwd()

#-------------------------------------------------------------------------------
# Run formatting
#-------------------------------------------------------------------------------

directories = []
for subDir, dirs, files in os.walk(sourceDir):
	directories.append(os.path.abspath(subDir))
	
for dir in directories:
	os.chdir(dir)
	subprocess.check_call([
	
		'clang-format',
		'-i',
		'-style=file',
		'*.cpp',
		'*.h'
	])
	
os.chdir(scriptDir)
