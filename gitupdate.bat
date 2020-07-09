git config --global core.autocrlf true
git pull origin master
git add -A
git commit -m "update"
git push origin master
rem git tag -a "FQNews-v1.0.6" -m "FQNews-v1.0.6"
git tag -a "ChromeGo-v20200709" -m "ChromeGo-v20200709"
git push origin --tags
pause