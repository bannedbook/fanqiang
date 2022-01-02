git config --global core.autocrlf true
git pull origin master
git add -A
git commit -m "update"
git push origin master
rem git tag -a "FQNews-v1.2.1" -m "FQNews-v1.2.1"
git tag -a "ChromeGo-latest" -m "ChromeGo-latest"
git push origin --tags
pause