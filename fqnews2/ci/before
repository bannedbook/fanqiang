#!/bin/bash
set -euo pipefail

mkdir -p "${ANDROID_HOME}"

if ! [ -d "${ANDROID_HOME}/tools" ]; then
  wget --quiet -O "${ANDROID_HOME}/tools.zip" https://dl.google.com/android/repository/sdk-tools-linux-3859397.zip
  unzip -qq "${ANDROID_HOME}/tools.zip" -d "${ANDROID_HOME}"
fi
rm -f "${ANDROID_HOME}/tools.zip"

LOGDIR="build/logs"
LOG="${LOGDIR}/sdkmanager.log"
mkdir -p "${LOGDIR}"

echo y | sdkmanager --update >>"${LOG}"
echo y | sdkmanager \
           "tools" \
           "platform-tools"
