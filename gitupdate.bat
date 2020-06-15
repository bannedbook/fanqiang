git config --global core.autocrlf true
git pull origin master
git add -A
git commit -m "update"
git push origin master
git tag -a "FQNews-v1.0.6" -m "FQNews-v1.0.6"
rem git tag -a "ChromeGo-v20200604" -m "ChromeGo-v20200604"
git push origin --tags
pause