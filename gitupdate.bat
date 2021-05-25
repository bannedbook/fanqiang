git config --global core.autocrlf true
git pull origin master
git add -A
git commit -m "update"
git push origin master
rem git tag -a "FQNews-v1.2.1" -m "FQNews-v1.2.1"
git tag -a "ChromeGo-v20210525" -m "ChromeGo-v20210525"
git push origin --tags
pause