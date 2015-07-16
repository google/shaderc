# Copyright 2015 The Shaderc Authors. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Tests for the expect module."""

from expect import get_object_filename
from nose.tools import assert_equal

def nosetest_get_object_name():
    """Tests get_object_filename()."""
    source_and_object_names = [
        ('a.vert', 'a.vert.spv'), ('b.frag', 'b.frag.spv'),
        ('c.tesc', 'c.tesc.spv'), ('d.tese', 'd.tese.spv'),
        ('e.geom', 'e.geom.spv'), ('f.comp', 'f.comp.spv'),
        ('file', 'file.spv'), ('file.', 'file.spv'),
        ('file.uk', 'file.spv'), ('file.vert.', 'file.vert.spv'),
        ('file.vert.bla', 'file.vert.spv')]
    actual_object_names = [
        get_object_filename(f[0]) for f in source_and_object_names]
    expected_object_names = [f[1] for f in source_and_object_names]

    assert_equal(actual_object_names, expected_object_names)
