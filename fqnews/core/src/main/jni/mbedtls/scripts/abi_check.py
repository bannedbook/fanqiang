#!/usr/bin/env python3
"""
This file is part of Mbed TLS (https://tls.mbed.org)

Copyright (c) 2018, Arm Limited, All Rights Reserved

Purpose

This script is a small wrapper around the abi-compliance-checker and
abi-dumper tools, applying them to compare the ABI and API of the library
files from two different Git revisions within an Mbed TLS repository.
The results of the comparison are formatted as HTML and stored at
a configurable location. Returns 0 on success, 1 on ABI/API non-compliance,
and 2 if there is an error while running the script.
Note: must be run from Mbed TLS root.
"""

import os
import sys
import traceback
import shutil
import subprocess
import argparse
import logging
import tempfile


class AbiChecker(object):

    def __init__(self, report_dir, old_rev, new_rev, keep_all_reports):
        self.repo_path = "."
        self.log = None
        self.setup_logger()
        self.report_dir = os.path.abspath(report_dir)
        self.keep_all_reports = keep_all_reports
        self.should_keep_report_dir = os.path.isdir(self.report_dir)
        self.old_rev = old_rev
        self.new_rev = new_rev
        self.mbedtls_modules = ["libmbedcrypto", "libmbedtls", "libmbedx509"]
        self.old_dumps = {}
        self.new_dumps = {}
        self.git_command = "git"
        self.make_command = "make"

    def check_repo_path(self):
        current_dir = os.path.realpath('.')
        root_dir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
        if current_dir != root_dir:
            raise Exception("Must be run from Mbed TLS root")

    def setup_logger(self):
        self.log = logging.getLogger()
        self.log.setLevel(logging.INFO)
        self.log.addHandler(logging.StreamHandler())

    def check_abi_tools_are_installed(self):
        for command in ["abi-dumper", "abi-compliance-checker"]:
            if not shutil.which(command):
                raise Exception("{} not installed, aborting".format(command))

    def get_clean_worktree_for_git_revision(self, git_rev):
        self.log.info(
            "Checking out git worktree for revision {}".format(git_rev)
        )
        git_worktree_path = tempfile.mkdtemp()
        worktree_process = subprocess.Popen(
            [self.git_command, "worktree", "add", git_worktree_path, git_rev],
            cwd=self.repo_path,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT
        )
        worktree_output, _ = worktree_process.communicate()
        self.log.info(worktree_output.decode("utf-8"))
        if worktree_process.returncode != 0:
            raise Exception("Checking out worktree failed, aborting")
        return git_worktree_path

    def build_shared_libraries(self, git_worktree_path):
        my_environment = os.environ.copy()
        my_environment["CFLAGS"] = "-g -Og"
        my_environment["SHARED"] = "1"
        make_process = subprocess.Popen(
            self.make_command,
            env=my_environment,
            cwd=git_worktree_path,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT
        )
        make_output, _ = make_process.communicate()
        self.log.info(make_output.decode("utf-8"))
        if make_process.returncode != 0:
            raise Exception("make failed, aborting")

    def get_abi_dumps_from_shared_libraries(self, git_ref, git_worktree_path):
        abi_dumps = {}
        for mbed_module in self.mbedtls_modules:
            output_path = os.path.join(
                self.report_dir, "{}-{}.dump".format(mbed_module, git_ref)
            )
            abi_dump_command = [
                "abi-dumper",
                os.path.join(
                    git_worktree_path, "library", mbed_module + ".so"),
                "-o", output_path,
                "-lver", git_ref
            ]
            abi_dump_process = subprocess.Popen(
                abi_dump_command,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT
            )
            abi_dump_output, _ = abi_dump_process.communicate()
            self.log.info(abi_dump_output.decode("utf-8"))
            if abi_dump_process.returncode != 0:
                raise Exception("abi-dumper failed, aborting")
            abi_dumps[mbed_module] = output_path
        return abi_dumps

    def cleanup_worktree(self, git_worktree_path):
        shutil.rmtree(git_worktree_path)
        worktree_process = subprocess.Popen(
            [self.git_command, "worktree", "prune"],
            cwd=self.repo_path,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT
        )
        worktree_output, _ = worktree_process.communicate()
        self.log.info(worktree_output.decode("utf-8"))
        if worktree_process.returncode != 0:
            raise Exception("Worktree cleanup failed, aborting")

    def get_abi_dump_for_ref(self, git_rev):
        git_worktree_path = self.get_clean_worktree_for_git_revision(git_rev)
        self.build_shared_libraries(git_worktree_path)
        abi_dumps = self.get_abi_dumps_from_shared_libraries(
            git_rev, git_worktree_path
        )
        self.cleanup_worktree(git_worktree_path)
        return abi_dumps

    def get_abi_compatibility_report(self):
        compatibility_report = ""
        compliance_return_code = 0
        for mbed_module in self.mbedtls_modules:
            output_path = os.path.join(
                self.report_dir, "{}-{}-{}.html".format(
                    mbed_module, self.old_rev, self.new_rev
                )
            )
            abi_compliance_command = [
                "abi-compliance-checker",
                "-l", mbed_module,
                "-old", self.old_dumps[mbed_module],
                "-new", self.new_dumps[mbed_module],
                "-strict",
                "-report-path", output_path
            ]
            abi_compliance_process = subprocess.Popen(
                abi_compliance_command,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT
            )
            abi_compliance_output, _ = abi_compliance_process.communicate()
            self.log.info(abi_compliance_output.decode("utf-8"))
            if abi_compliance_process.returncode == 0:
                compatibility_report += (
                    "No compatibility issues for {}\n".format(mbed_module)
                )
                if not self.keep_all_reports:
                    os.remove(output_path)
            elif abi_compliance_process.returncode == 1:
                compliance_return_code = 1
                self.should_keep_report_dir = True
                compatibility_report += (
                    "Compatibility issues found for {}, "
                    "for details see {}\n".format(mbed_module, output_path)
                )
            else:
                raise Exception(
                    "abi-compliance-checker failed with a return code of {},"
                    " aborting".format(abi_compliance_process.returncode)
                )
            os.remove(self.old_dumps[mbed_module])
            os.remove(self.new_dumps[mbed_module])
        if not self.should_keep_report_dir and not self.keep_all_reports:
            os.rmdir(self.report_dir)
        self.log.info(compatibility_report)
        return compliance_return_code

    def check_for_abi_changes(self):
        self.check_repo_path()
        self.check_abi_tools_are_installed()
        self.old_dumps = self.get_abi_dump_for_ref(self.old_rev)
        self.new_dumps = self.get_abi_dump_for_ref(self.new_rev)
        return self.get_abi_compatibility_report()


def run_main():
    try:
        parser = argparse.ArgumentParser(
            description=(
                """This script is a small wrapper around the
                abi-compliance-checker and abi-dumper tools, applying them
                to compare the ABI and API of the library files from two
                different Git revisions within an Mbed TLS repository.
                The results of the comparison are formatted as HTML and stored
                at a configurable location. Returns 0 on success, 1 on ABI/API
                non-compliance, and 2 if there is an error while running the
                script. Note: must be run from Mbed TLS root."""
            )
        )
        parser.add_argument(
            "-r", "--report-dir", type=str, default="reports",
            help="directory where reports are stored, default is reports",
        )
        parser.add_argument(
            "-k", "--keep-all-reports", action="store_true",
            help="keep all reports, even if there are no compatibility issues",
        )
        parser.add_argument(
            "-o", "--old-rev", type=str, help="revision for old version",
            required=True
        )
        parser.add_argument(
            "-n", "--new-rev", type=str, help="revision for new version",
            required=True
        )
        abi_args = parser.parse_args()
        abi_check = AbiChecker(
            abi_args.report_dir, abi_args.old_rev,
            abi_args.new_rev, abi_args.keep_all_reports
        )
        return_code = abi_check.check_for_abi_changes()
        sys.exit(return_code)
    except Exception:
        traceback.print_exc()
        sys.exit(2)


if __name__ == "__main__":
    run_main()
