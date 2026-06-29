#!/bin/bash
set -e

cd "$(dirname "$0")"

scons -c > /dev/null 2>&1 || true
scons -Q custom_api_file=../extension_api.json
