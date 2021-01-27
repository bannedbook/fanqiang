git config --global core.autocrlf true
git pull origin master
git add -A
git commit -m "update"
git push origin master
git tag -a "FQNews-v1.2.1" -m "FQNews-v1.2.1"
rem git tag -a "ChromeGo-v20210120" -m "ChromeGo-v20210120"
git push origin --tags
pause