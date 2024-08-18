fastlane documentation
================
# Installation

Make sure you have the latest version of the Xcode command line tools installed:

```
xcode-select --install
```

Install _fastlane_ using
```
[sudo] gem install fastlane -NV
```
or alternatively using `brew install fastlane`

# Available Actions
## Android
### android get_latest_version
```
fastlane android get_latest_version
```
Fetch latest version code from Google Play
### android bump_version_code
```
fastlane android bump_version_code
```
Bumps version code in gradle file
### android build_play_bundle
```
fastlane android build_play_bundle
```
Build play bundle
### android deploy
```
fastlane android deploy
```
Deploy a new version to the Google Play
### android validate_deploy
```
fastlane android validate_deploy
```
Validate deployment of a new version to the Google Play
### android build_and_deploy
```
fastlane android build_and_deploy
```
Build and deploy a new version to the Google Play
### android build_and_validate
```
fastlane android build_and_validate
```
Build and valdidate deployment of a new version to the Google Play

----

This README.md is auto-generated and will be re-generated every time [fastlane](https://fastlane.tools) is run.
More information about fastlane can be found on [fastlane.tools](https://fastlane.tools).
The documentation of fastlane can be found on [docs.fastlane.tools](https://docs.fastlane.tools).
