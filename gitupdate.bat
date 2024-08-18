git config --global core.autocrlf true
git pull origin master
git add -A
git commit -m "update"
git push origin master
git tag -a "FQNews-v1.3.8" -m "FQNews2-v1.3.8"
rem git tag -a "ChromeGo-latest" -m "ChromeGo-latest"
git push origin --tags
pause