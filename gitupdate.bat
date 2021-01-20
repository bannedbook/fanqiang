git config --global core.autocrlf true
git pull origin master
git add -A
git commit -m "update"
git push origin master
rem git tag -a "FQNews-v1.2.0" -m "FQNews-v1.2.0"
git tag -a "ChromeGo-v20210120" -m "ChromeGo-v20210120"
git push origin --tags
pause