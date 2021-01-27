#!/usr/bin/env bash
set -x
git config --global core.autocrlf true
git pull origin master
git add -A
git commit -m "update"
git push origin master
git tag -a "FQNews-v1.1.0" -m "FQNews-v1.1.0"
# git tag -a "ChromeGo-v20200709" -m "ChromeGo-v20200709"
git push origin --tags
