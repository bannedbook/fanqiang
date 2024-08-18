#!/bin/bash -eu

TARGET="${1:-HEAD}"

current_default="$(git describe --tags --abbrev=0 "${TARGET}")"

echo >&2 -n "Current version [${current_default}]: "
read -r current_in

if [ -z "${current_in}" ]; then
  CURRENT_VERSION="${current_default}"
else
  CURRENT_VERSION="${current_in}"
fi

next_default="$(grep "versionName" app/build.gradle.kts | sed "s|\s*versionName = \"\(.*\)\"|\\1|")"
echo >&2 -n "Next version [${next_default}]: "
read -r next_in

if [ -z "${next_in}" ]; then
  NEXT_VERSION="${next_default}"
else
  NEXT_VERSION="${next_in}"
fi

CURRENT_CODE="$(grep "versionCode" app/build.gradle.kts | sed "s|\s*versionCode = \([0-9]\+\)|\\1|")"
echo >&2 "Current code ${CURRENT_CODE}"

next_code_default=$(( CURRENT_CODE+1 ))

echo >&2 -n "Next code [${next_code_default}]: "
read -r next_code_in

if [ -z "${next_code_in}" ]; then
  NEXT_CODE="${next_code_default}"
else
  NEXT_CODE="${next_code_in}"
fi

# Get rid of these to make the build reproducible
ci/delete-unwanted-langs

read -r -p "Update locales_config.xml? [y/N] " response
if [[ "$response" =~ ^[yY]$ ]]
then
  ./gradlew --no-configuration-cache :app:generateLocalesConfig
  git add app/src/main/res/xml/locales_config.xml
fi

CL="# ${NEXT_VERSION}
$(git shortlog -w76,2,9 --max-parents=1 --format='* [%h] %s' "${CURRENT_VERSION}..HEAD")
"

tmpfile="$(mktemp)"

echo "${CL}" > "${tmpfile}"

sensible-editor "${tmpfile}"

echo >&2 "Changelog for [${NEXT_VERSION}]:"
cat >&2 "${tmpfile}"

read -r -p "Write changelog? [y/N] " response
if [[ "$response" =~ ^[yY]$ ]]
then
  # Playstore has a limit
  head --bytes=500 "${tmpfile}" >"fastlane/metadata/android/en-US/changelogs/${NEXT_CODE}.txt"

  PREV=""
  if [ -f CHANGELOG.md ]; then
    PREV="$(cat CHANGELOG.md)"
  fi

  cat >CHANGELOG.md <<EOF
$(cat "${tmpfile}")

${PREV}
EOF
fi

read -r -p "Update gradle versions? [y/N] " response
if [[ "$response" =~ ^[yY]$ ]]
then
  sed -i "s|\(\s*versionCode = \)[0-9]\+|\\1${NEXT_CODE}|" app/build.gradle.kts
  sed -i "s|\(\s*versionName = \).*|\\1\"${NEXT_VERSION}\"|" app/build.gradle.kts
fi

echo "Verifying build"
./gradlew check pixel2api30DebugAndroidTest || echo >&2 "Build failed"

read -r -p "Commit changes? [y/N] " response
if [[ "$response" =~ ^[yY]$ ]]
then
  git add "fastlane/metadata/android/en-US/changelogs/${NEXT_CODE}.txt"
  git add app/build.gradle.kts
  git add CHANGELOG.md
  git diff --staged
  git commit -m "Releasing ${NEXT_VERSION}"
fi

read -r -p "Make tag? [y/N] " response
if [[ "$response" =~ ^[yY]$ ]]
then
  git tag -asm "$(cat "${tmpfile}")" "${NEXT_VERSION}"
fi

git checkout app/src/main/res fastlane/metadata/android

read -r -p "Post to feed? [y/N] " response
if [[ "$response" =~ ^[yY]$ ]]
then
  scripts/changelog-to-hugo.main.kts  ../feeder-news/content/posts/ "${NEXT_VERSION}"
  pushd ../feeder-news
  git add content/posts/
  git diff --staged
  git commit -m "Released ${NEXT_VERSION}"
  popd
fi

read -r -p "Push the lot? [y/N] " response
if [[ "$response" =~ ^[yY]$ ]]
then
  git push --follow-tags
  pushd ../feeder-news
  git push
  popd
fi
