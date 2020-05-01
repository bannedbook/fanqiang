git config --global core.autocrlf true
git pull origin master
git add -A
git commit -m "update"
git push origin master
git tag -a "FQNews-v1.0.0" -m "FQNews-v1.0.0"
git push origin --tags
pause