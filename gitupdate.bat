git config --global core.autocrlf true
git pull origin master
git add -A
git commit -m "update"
git push origin master
git tag -a "FQNews-v5.0.6.8" -m "FQNews v5.0.6.8"
git push origin --tags
pause