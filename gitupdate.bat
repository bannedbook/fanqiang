git config --global core.autocrlf true
git pull origin master
git add -A
git commit -m "update"
git push origin master
rem git tag -a "FQNews-v1.1.3" -m "FQNews-v1.1.3"
git tag -a "ChromeGo-v20201103" -m "ChromeGo-v20201103"
git push origin --tags
pause