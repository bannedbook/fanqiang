git config --global core.autocrlf true
git pull origin master
git add -A
git commit -m "update"
git push origin master
git tag -a "FQNews-v1.1.3" -m "FQNews-v1.1.3"
rem git tag -a "ChromeGo-v20200818" -m "ChromeGo-v20200818"
git push origin --tags
pause