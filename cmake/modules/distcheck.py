#!/usr/bin/python
# -*- coding: utf-8 -*-

# Copyright (C) 2009 Collabora Limited <http://www.collabora.co.uk>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

import os
import re
import shutil
import sys
import tarfile
import subprocess

project = sys.argv[1]
version = sys.argv[2]
srcDir = sys.argv[3]
buildDir = sys.argv[4]

tarName = project + '-' + version
distfilesDir = buildDir + "/distfiles"

try:
    os.mkdir( distfilesDir )
except:
    print "Delete " + distfilesDir + " before running distcheck again."
    exit( 1 )

cloneSrcDir = distfilesDir + "/" + tarName
cloneBuildDir = cloneSrcDir + "_build"

os.chdir( buildDir )
subprocess.call( ["git", "clone", srcDir,  cloneSrcDir ] )
os.chdir( cloneSrcDir )
hashOfTar = subprocess.Popen(["git", "log", "--pretty=oneline", "-n1"], stdout=subprocess.PIPE).communicate()[0].strip()
os.mkdir( cloneBuildDir )
os.chdir( cloneBuildDir )

originalCache = open( buildDir + "/CMakeCache.txt" )
newCache = open( cloneBuildDir + "/CMakeCache.txt", 'w' )
reSrcDir = re.compile( srcDir + "$" )
reBuildDir = re.compile( buildDir + "$" )
for line in originalCache.xreadlines():
    line = reSrcDir.sub( cloneSrcDir, line )
    newCache.write( reBuildDir.sub( cloneBuildDir, line ) )
newCache.close()

if subprocess.call( ["cmake", cloneSrcDir] ) == 0:
    if subprocess.call( ["make"] ) == 0:
        docStatus = ( subprocess.call( ["make", "doxygen-doc"] ) == 0 )
        shutil.move( cloneBuildDir + "/doc", cloneSrcDir )

        testStatus = ( subprocess.call( ["make", "test"] ) == 0 )

        subprocess.call( ["rm", "-rf", cloneSrcDir + "/.git*" ] )
        tarPath = buildDir + "/" + tarName + ".tar.gz"
        tar = tarfile.open( tarPath, "w|gz" )
        tar.add( cloneSrcDir )
        tar.close()
        print "Tarball " + tarPath + " built."
        print "Remember to tag " + hashOfTar
        if not docStatus:
            print "WARNING: Problem building documentation."
        if not testStatus:
            print "WARNING: One or more tests failed."
    else:
        print "Building failed"
        exit( 1 )
else:
    print "CMake failed"
    exit( 1 )
