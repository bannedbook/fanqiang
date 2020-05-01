#!/usr/bin/env python3
"""
This file is part of Mbed TLS (https://tls.mbed.org)

Copyright (c) 2018, Arm Limited, All Rights Reserved

Purpose

This script checks the current state of the source code for minor issues,
including incorrect file permissions, presence of tabs, non-Unix line endings,
trailing whitespace, presence of UTF-8 BOM, and TODO comments.
Note: requires python 3, must be run from Mbed TLS root.
"""

import os
import argparse
import logging
import codecs
import sys


class IssueTracker(object):
    """Base class for issue tracking. Issues should inherit from this and
    overwrite either issue_with_line if they check the file line by line, or
    overwrite check_file_for_issue if they check the file as a whole."""

    def __init__(self):
        self.heading = ""
        self.files_exemptions = []
        self.files_with_issues = {}

    def should_check_file(self, filepath):
        for files_exemption in self.files_exemptions:
            if filepath.endswith(files_exemption):
                return False
        return True

    def issue_with_line(self, line):
        raise NotImplementedError

    def check_file_for_issue(self, filepath):
        with open(filepath, "rb") as f:
            for i, line in enumerate(iter(f.readline, b"")):
                self.check_file_line(filepath, line, i + 1)

    def record_issue(self, filepath, line_number):
        if filepath not in self.files_with_issues.keys():
            self.files_with_issues[filepath] = []
        self.files_with_issues[filepath].append(line_number)

    def check_file_line(self, filepath, line, line_number):
        if self.issue_with_line(line):
            self.record_issue(filepath, line_number)

    def output_file_issues(self, logger):
        if self.files_with_issues.values():
            logger.info(self.heading)
            for filename, lines in sorted(self.files_with_issues.items()):
                if lines:
                    logger.info("{}: {}".format(
                        filename, ", ".join(str(x) for x in lines)
                    ))
                else:
                    logger.info(filename)
            logger.info("")


class PermissionIssueTracker(IssueTracker):

    def __init__(self):
        super().__init__()
        self.heading = "Incorrect permissions:"

    def check_file_for_issue(self, filepath):
        if not (os.access(filepath, os.X_OK) ==
                filepath.endswith((".sh", ".pl", ".py"))):
            self.files_with_issues[filepath] = None


class EndOfFileNewlineIssueTracker(IssueTracker):

    def __init__(self):
        super().__init__()
        self.heading = "Missing newline at end of file:"

    def check_file_for_issue(self, filepath):
        with open(filepath, "rb") as f:
            if not f.read().endswith(b"\n"):
                self.files_with_issues[filepath] = None


class Utf8BomIssueTracker(IssueTracker):

    def __init__(self):
        super().__init__()
        self.heading = "UTF-8 BOM present:"

    def check_file_for_issue(self, filepath):
        with open(filepath, "rb") as f:
            if f.read().startswith(codecs.BOM_UTF8):
                self.files_with_issues[filepath] = None


class LineEndingIssueTracker(IssueTracker):

    def __init__(self):
        super().__init__()
        self.heading = "Non Unix line endings:"

    def issue_with_line(self, line):
        return b"\r" in line


class TrailingWhitespaceIssueTracker(IssueTracker):

    def __init__(self):
        super().__init__()
        self.heading = "Trailing whitespace:"
        self.files_exemptions = [".md"]

    def issue_with_line(self, line):
        return line.rstrip(b"\r\n") != line.rstrip()


class TabIssueTracker(IssueTracker):

    def __init__(self):
        super().__init__()
        self.heading = "Tabs present:"
        self.files_exemptions = [
            "Makefile", "generate_visualc_files.pl"
        ]

    def issue_with_line(self, line):
        return b"\t" in line


class MergeArtifactIssueTracker(IssueTracker):

    def __init__(self):
        super().__init__()
        self.heading = "Merge artifact:"

    def issue_with_line(self, filepath, line):
        # Detect leftover git conflict markers.
        if line.startswith(b'<<<<<<< ') or line.startswith(b'>>>>>>> '):
            return True
        if line.startswith(b'||||||| '): # from merge.conflictStyle=diff3
            return True
        if line.rstrip(b'\r\n') == b'=======' and \
           not filepath.endswith('.md'):
            return True
        return False

    def check_file_line(self, filepath, line, line_number):
        if self.issue_with_line(filepath, line):
            self.record_issue(filepath, line_number)

class TodoIssueTracker(IssueTracker):

    def __init__(self):
        super().__init__()
        self.heading = "TODO present:"
        self.files_exemptions = [
            os.path.basename(__file__),
            "benchmark.c",
            "pull_request_template.md",
        ]

    def issue_with_line(self, line):
        return b"todo" in line.lower()


class IntegrityChecker(object):

    def __init__(self, log_file):
        self.check_repo_path()
        self.logger = None
        self.setup_logger(log_file)
        self.files_to_check = (
            ".c", ".h", ".sh", ".pl", ".py", ".md", ".function", ".data",
            "Makefile", "CMakeLists.txt", "ChangeLog"
        )
        self.excluded_directories = ['.git', 'mbed-os']
        self.excluded_paths = list(map(os.path.normpath, [
            'cov-int',
            'examples',
        ]))
        self.issues_to_check = [
            PermissionIssueTracker(),
            EndOfFileNewlineIssueTracker(),
            Utf8BomIssueTracker(),
            LineEndingIssueTracker(),
            TrailingWhitespaceIssueTracker(),
            TabIssueTracker(),
            MergeArtifactIssueTracker(),
            TodoIssueTracker(),
        ]

    def check_repo_path(self):
        if not all(os.path.isdir(d) for d in ["include", "library", "tests"]):
            raise Exception("Must be run from Mbed TLS root")

    def setup_logger(self, log_file, level=logging.INFO):
        self.logger = logging.getLogger()
        self.logger.setLevel(level)
        if log_file:
            handler = logging.FileHandler(log_file)
            self.logger.addHandler(handler)
        else:
            console = logging.StreamHandler()
            self.logger.addHandler(console)

    def prune_branch(self, root, d):
        if d in self.excluded_directories:
            return True
        if os.path.normpath(os.path.join(root, d)) in self.excluded_paths:
            return True
        return False

    def check_files(self):
        for root, dirs, files in os.walk("."):
            dirs[:] = sorted(d for d in dirs if not self.prune_branch(root, d))
            for filename in sorted(files):
                filepath = os.path.join(root, filename)
                if not filepath.endswith(self.files_to_check):
                    continue
                for issue_to_check in self.issues_to_check:
                    if issue_to_check.should_check_file(filepath):
                        issue_to_check.check_file_for_issue(filepath)

    def output_issues(self):
        integrity_return_code = 0
        for issue_to_check in self.issues_to_check:
            if issue_to_check.files_with_issues:
                integrity_return_code = 1
            issue_to_check.output_file_issues(self.logger)
        return integrity_return_code


def run_main():
    parser = argparse.ArgumentParser(
        description=(
            "This script checks the current state of the source code for "
            "minor issues, including incorrect file permissions, "
            "presence of tabs, non-Unix line endings, trailing whitespace, "
            "presence of UTF-8 BOM, and TODO comments. "
            "Note: requires python 3, must be run from Mbed TLS root."
        )
    )
    parser.add_argument(
        "-l", "--log_file", type=str, help="path to optional output log",
    )
    check_args = parser.parse_args()
    integrity_check = IntegrityChecker(check_args.log_file)
    integrity_check.check_files()
    return_code = integrity_check.output_issues()
    sys.exit(return_code)


if __name__ == "__main__":
    run_main()
