#!/usr/bin/env python
# -*- coding: utf-8 -*-

#-------------------------------------------------------------------------------
# Format sources using clang-format
# Usage : Format.py
# 
# Gwennaël Arbona 2022
#-------------------------------------------------------------------------------

import glob
import json
import os
import re
import subprocess

#-------------------------------------------------------------------------------
# Define run settings
#-------------------------------------------------------------------------------

sourceDir = os.path.join('..', 'Source')
scriptDir = os.getcwd()

#-------------------------------------------------------------------------------
# Run formatting
#-------------------------------------------------------------------------------
	
for root, directories, filenames in os.walk(sourceDir):
	for filename in filenames:		
		if re.match('.*\.((h)|(c)|(cpp))$', filename):
			subprocess.check_call([				
				'clang-format',
				'-i',
				'-style=file',
				str(os.path.join(root, filename))
			])
	
os.chdir(scriptDir)
