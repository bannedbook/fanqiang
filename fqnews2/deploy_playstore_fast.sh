#!/bin/bash -eu

LATEST_TAG="$(git describe --tags "$(git rev-list --tags --max-count=1)")"
CURRENT_VERSION="$(git describe --tags)"

if [[ "${1:-}" == "--dry-run" ]] && [[ "${LATEST_TAG}" == "${CURRENT_VERSION}" ]]; then
  # CI runs master and tag pipelines concurrently and fastlane will conflict if run concurrently
  echo "${CURRENT_VERSION} is a tag but --dry-run was specified - not doing anything"
elif [[ "${1:-}" == "--dry-run" ]] || [[ "${LATEST_TAG}" != "${CURRENT_VERSION}" ]]; then
  echo "${CURRENT_VERSION} is not tag - validating deployment"
  if [[ "${CURRENT_VERSION}" =~ ^[0-9.]*$ ]]; then
    echo "${CURRENT_VERSION} is a production release"
    fastlane validate_deploy track:production
  else
    echo "${CURRENT_VERSION} is a beta release"
    fastlane validate_deploy track:beta
  fi
else
  echo "${CURRENT_VERSION} is a tag - deploying to store!"
  if [[ "${CURRENT_VERSION}" =~ ^[0-9.]*$ ]]; then
    echo "${CURRENT_VERSION} is a production release"
    fastlane deploy track:production
  else
    echo "${CURRENT_VERSION} is a beta release"
    fastlane deploy track:beta
  fi
fi
