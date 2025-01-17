# vim:ts=4:sw=4:et:
# Copyright (c) Facebook, Inc. and its affiliates.
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

# no unicode literals
from __future__ import absolute_import, division, print_function

import os
import os.path

import WatchmanTestCase


@WatchmanTestCase.expand_matrix
class TestMatch(WatchmanTestCase.WatchmanTestCase):
    def test_match(self):
        root = self.mkdtemp()
        self.touchRelative(root, "foo.c")
        self.touchRelative(root, "bar.txt")
        os.mkdir(os.path.join(root, "foo"))
        self.touchRelative(root, "foo", ".bar.c")
        self.touchRelative(root, "foo", "baz.c")

        self.watchmanCommand("watch", root)

        self.assertFileList(
            root, ["bar.txt", "foo.c", "foo", "foo/.bar.c", "foo/baz.c"]
        )

        res = self.watchmanCommand(
            "query", root, {"expression": ["match", "*.c"], "fields": ["name"]}
        )
        self.assertFileListsEqual(res["files"], ["foo.c", "foo/baz.c"])

        res = self.watchmanCommand(
            "query",
            root,
            {"expression": ["match", "*.c", "wholename"], "fields": ["name"]},
        )
        self.assertFileListsEqual(res["files"], ["foo.c"])

        res = self.watchmanCommand(
            "query",
            root,
            {"expression": ["match", "foo/*.c", "wholename"], "fields": ["name"]},
        )
        self.assertFileListsEqual(res["files"], ["foo/baz.c"])

        res = self.watchmanCommand(
            "query",
            root,
            {"expression": ["match", "foo/*.c", "wholename"], "fields": ["name"]},
        )
        self.assertFileListsEqual(res["files"], ["foo/baz.c"])

        res = self.watchmanCommand(
            "query",
            root,
            {"expression": ["match", "**/*.c", "wholename"], "fields": ["name"]},
        )
        self.assertFileListsEqual(res["files"], ["foo.c", "foo/baz.c"])

        res = self.watchmanCommand(
            "query",
            root,
            {
                "expression": [
                    "match",
                    "**/*.c",
                    "wholename",
                    {"includedotfiles": True},
                ],
                "fields": ["name"],
            },
        )
        self.assertFileListsEqual(res["files"], ["foo.c", "foo/.bar.c", "foo/baz.c"])

        res = self.watchmanCommand(
            "query",
            root,
            {"expression": ["match", "foo/**/*.c", "wholename"], "fields": ["name"]},
        )
        self.assertFileListsEqual(res["files"], ["foo/baz.c"])

        res = self.watchmanCommand(
            "query",
            root,
            {"expression": ["match", "FOO/*.c", "wholename"], "fields": ["name"]},
        )
        if self.isCaseInsensitive():
            self.assertFileListsEqual(res["files"], ["foo/baz.c"])
        else:
            self.assertFileListsEqual(res["files"], [])

        res = self.watchmanCommand(
            "query",
            root,
            {
                "expression": ["match", "FOO/*.c", "wholename"],
                "case_sensitive": True,
                "fields": ["name"],
            },
        )
        self.assertFileListsEqual(res["files"], [])

        res = self.watchmanCommand(
            "query",
            root,
            {
                "expression": ["match", "FOO/*.c", "wholename"],
                "case_sensitive": False,
                "fields": ["name"],
            },
        )
        self.assertFileListsEqual(res["files"], ["foo/baz.c"])
