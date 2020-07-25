#!/usr/bin/python

# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This zips the SDK and uploads it to Google Storage when run on a buildbot.

import os
import subprocess
import sys
import utils


GSUTIL = '/b/build/scripts/subordinate/gsutil'
GS_SITE = 'gs://'
GS_DIR = 'dart-dump-render-tree'
LATEST = 'latest'
SDK = 'sdk'

def ExecuteCommand(cmd):
  """Execute a command in a subprocess.
  """
  print 'Executing: ' + ' '.join(cmd)
  pipe = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  output = pipe.communicate()
  if pipe.returncode != 0:
    print 'Execution failed: ' + str(output)
  return (pipe.returncode, output)


def UploadArchive(source, target):
  """Upload an archive zip file to Google storage.
  """
  # Upload file.
  cmd = [GSUTIL, 'cp', source, target]
  (status, output) = ExecuteCommand(cmd)
  if status != 0:
    return status
  print 'Uploaded: ' + output[0]

  cmd = [GSUTIL, 'setacl', 'public-read', target]
  (status, output) = ExecuteCommand(cmd)
  return status

def GetSVNRevision():
  p = subprocess.Popen(['svn', 'info'], stdout = subprocess.PIPE,
      stderr = subprocess.STDOUT, close_fds=True)
  output, not_used = p.communicate()
  for line in output.split('\n'):
    if 'Revision' in line:
      return (line.strip().split())[1]
  return None


def Usage(progname):
  sys.stderr.write('Usage: %s path_to_sdk\n' % progname)


def main(argv):
  if not os.path.exists(argv[1]):
    sys.stderr.write('Path not found: %s\n' % argv[1])
    Usage(argv[0])
    return 1
  if not os.path.exists(GSUTIL):
    exit(0)
  revision = GetSVNRevision()
  if revision is None:
    sys.stderr.write('Unable to find SVN revision.\n')
    return 1
  os.chdir(argv[1])
  # TODO(dgrove) - deal with architectures that are not ia32.
  sdk_name = 'dart-' + utils.GuessOS() + '-' + revision + '.zip'
  sdk_file = '../' + sdk_name
  ExecuteCommand(['zip', '-yr', sdk_file, '.'])
  UploadArchive(sdk_file, GS_SITE + os.path.join(GS_DIR, SDK, sdk_name))
  latest_name = 'dart-' + utils.GuessOS() + '-latest' + '.zip'
  UploadArchive(sdk_file, GS_SITE + os.path.join(GS_DIR, SDK, latest_name))


if __name__ == '__main__':
  sys.exit(main(sys.argv))


