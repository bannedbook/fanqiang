#!/bin/bash -eu

if [ -n "${SERVICEACCOUNTJSON:-}" ]; then
  cat > app/creds.b64 <<EOF
${SERVICEACCOUNTJSON}
EOF
fi

base64 --ignore-garbage --decode app/creds.b64 > app/creds.json

sed -i "s|/home/jonas/.ssh/service.json|$(pwd)/app/creds.json|" fastlane/Appfile

if [ -n "${KEYSTORE:-}" ]; then
  cat > keystore.b64 <<EOF
${KEYSTORE}
EOF

  base64 --ignore-garbage --decode keystore.b64 > keystore

  cat >> gradle.properties <<EOF
STORE_FILE=$(pwd)/keystore
STORE_PASSWORD=${KEYSTOREPASSWORD}
KEY_ALIAS=${KEYALIAS}
KEY_PASSWORD=${KEYPASSWORD}
EOF

fi

# Delete unsupported google play store languages
ci/delete-unwanted-langs
