#!/bin/bash

# Delete unsupported google play store languages
# Want this done in git so that builds are reproducible
paths=(
    "fastlane/metadata/android/bs-BA"
    "fastlane/metadata/android/eo"
    "fastlane/metadata/android/tok"
    "app/src/main/res/values-tok"
    "fastlane/metadata/android/gl"
 #   "app/src/main/res/values-gl"
    "fastlane/metadata/android/eu"
    "app/src/main/res/values-eu"
    "fastlane/metadata/android/pt"
    "app/src/main/res/values-pt"
)

for p in "${paths[@]}"
do
    git rm -rf "$p" --ignore-unmatch
done
