# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import sys
import os
import exceptions
import itertools
import re


def GetNinjaOutputDirectory(chrome_root, configuration=None):
  """Returns <chrome_root>/<output_dir>/(Release|Debug).

  If either of the following environment variables are set, their
  value is used to determine the output directory:
    1. CHROMIUM_OUT_DIR environment variable.
    2. GYP_GENERATOR_FLAGS environment variable output_dir property.

  Otherwise, all directories starting with the word out are examined.

  The output directory must contain {configuration}/build.ninja (if
  configuration is None, both Debug and Release will be checked).

  The configuration chosen is the one most recently generated/built,
  but can be overriden via the <configuration> parameter.
  """

  output_dirs = []
  if ('CHROMIUM_OUT_DIR' in os.environ and
      os.path.isdir(os.path.join(chrome_root, os.environ['CHROMIUM_OUT_DIR']))):
    output_dirs = [os.environ['CHROMIUM_OUT_DIR']]
  if not output_dirs:
    generator_flags = os.getenv('GYP_GENERATOR_FLAGS', '').split(' ')
    for flag in generator_flags:
      name_value = flag.split('=', 1)
      if (len(name_value) == 2 and name_value[0] == 'output_dir' and
          os.path.isdir(os.path.join(chrome_root, name_value[1]))):
        output_dirs = [name_value[1]]
  if not output_dirs:
    for f in os.listdir(chrome_root):
      if re.match(r'out\b', f):
        out = os.path.realpath(os.path.join(chrome_root, f))
        if os.path.isdir(out):
          output_dirs.append(os.path.relpath(out, start = chrome_root))

  configs = ['Debug', 'Release', 'Default']
  if configuration:
    configs = [configuration]

  def generate_paths():
    for out_dir, config in itertools.product(output_dirs, configs):
      path = os.path.join(chrome_root, out_dir, config)
      if os.path.exists(os.path.join(path, 'build.ninja')):
        yield path

  def approx_directory_mtime(path):
    # This is a heuristic; don't recurse into subdirectories.
    paths = [path] + [os.path.join(path, f) for f in os.listdir(path)]
    return max(os.path.getmtime(p) for p in paths)

  try:
    return max(generate_paths(), key=approx_directory_mtime)
  except ValueError:
    raise exceptions.RuntimeError(
      'Unable to find a valid ninja output directory.')

if __name__ == '__main__':
  if len(sys.argv) != 2:
    raise exceptions.RuntimeError('Expected a single path argument.')
  print GetNinjaOutputDirectory(sys.argv[1])
