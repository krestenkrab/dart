#!/usr/bin/env python
#
# Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
# for details. All rights reserved. Use of this source code is governed by a
# BSD-style license that can be found in the LICENSE file.

# Gets or updates our local build of Dartium. This is used for testing Dart
# apps, without the need to build Dartium locally


import os
import sys
import platform
import errno
import tempfile
import shutil
import subprocess

def NormJoin(path1, path2):
  return os.path.normpath(os.path.join(path1, path2))

# Change into the dart directory as we want the project to be rooted here.
dart_src = NormJoin(os.path.dirname(sys.argv[0]), os.pardir)
os.chdir(dart_src)

GSUTIL_DIR = 'third_party/gsutil/20110627'
GSUTIL = GSUTIL_DIR + '/gsutil'
DARTIUM_DIR = 'client/tests/dartium'
VERSION = DARTIUM_DIR + '/BUILD_VERSION'

sys.path.append(GSUTIL_DIR + '/boto')
import boto


def execute_command(*cmd):
  """Execute a command in a subprocess."""
  pipe = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  output, error = pipe.communicate()
  if pipe.returncode != 0:
    raise Exception('Execution of "%s" failed: %s' % (cmd, str(output)))
  return output


def execute_command_visible(*cmd):
  """Execute a command in a subprocess, but show stdout/stderr."""
  result = subprocess.call(cmd, stdout=sys.stdout, stderr=sys.stderr,
                           stdin=sys.stdin)
  if result != 0:
    raise Exception('Execution of "%s" failed' % cmd)


def gsutil(*cmd):
  return execute_command(GSUTIL, *cmd)


def gsutil_visible(*cmd):
  execute_command_visible(GSUTIL, *cmd)


def has_boto_config():
  """Returns true if boto config exists."""

  config_paths = boto.pyami.config.BotoConfigLocations
  if 'AWS_CREDENTIAL_FILE' in os.environ:
    config_paths.append(os.environ['AWS_CREDENTIAL_FILE'])
  for config_path in config_paths:
    if os.path.exists(config_path):
      return True

  return False


def in_runhooks():
  '''True if this script was called by "gclient runhooks" or "gclient sync"'''
  return 'runhooks' in sys.argv


def ensure_config():
  # If ~/.boto doesn't exist, tell the user to run "gsutil config"
  if not has_boto_config():
    print >>sys.stderr, '''
*******************************************************************************
* WARNING: Can't download Dartium binaries! These are required to test client.
* You need to do a one-time configuration step to access Google Storage.
* Please run this command and follow the instructions:
*     %s config
*
* NOTE: When prompted you can leave "project-id" blank. Just hit enter.
*******************************************************************************
''' % GSUTIL
    sys.exit(1)


def main():
  system = platform.system()
  if system == 'Darwin':
    osname = 'mac'
  elif system == 'Linux':
    osname = 'lucid64'
  else:
    print >>sys.stderr, ('WARNING: platform "%s" does not support'
        'DumpRenderTree for tests') % system
    return 1

  ensure_config()

  # Query the last known good build
  latest = gsutil('ls', 'gs://dashium-archive/latest/dashium-%s-full-*.zip' %
                  osname).split()[-1]

  # Check if we need to update the file
  if os.path.exists(VERSION):
    v = open(VERSION, 'r').read()
    if v == latest:
      if not in_runhooks():
        print 'Dartium is up to date.\nVersion: ' + latest
      return 0 # up to date

  if os.path.exists(DARTIUM_DIR):
    print 'Removing old dartium tree %s' % DARTIUM_DIR
    shutil.rmtree(DARTIUM_DIR)

  # download the zip file to a temporary path, and unzip to the target location
  temp_dir = tempfile.mkdtemp()
  try:
    temp_zip = temp_dir + '/dashium.zip'
    # It's nice to show download progress
    gsutil_visible('cp', latest, temp_zip)

    execute_command('unzip', temp_zip, '-d', temp_dir)
    unzipped_dir = temp_dir + '/' + os.path.basename(latest)[:-4] # remove .zip
    shutil.move(unzipped_dir, DARTIUM_DIR)
  finally:
    shutil.rmtree(temp_dir)

  # create the version stamp
  v = open(VERSION, 'w')
  v.write(latest)
  v.close()

  print 'Successfully downloaded to %s' % DARTIUM_DIR
  return 0

if __name__ == '__main__':
  sys.exit(main())