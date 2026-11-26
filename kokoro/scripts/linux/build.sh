#!/bin/bash

# Copyright (C) 2020 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Linux Build Script.

set -e # Fail on any error.

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd )"
ROOT_DIR="$( cd "${SCRIPT_DIR}/../../.." >/dev/null 2>&1 && pwd )"

CONFIG=$1
COMPILER=$2

# --privileged is required for some sanitizer builds, as they seem to require
# PTRACE privileges
# Translate a path under KOKORO_ROOT_DIR to the corresponding host VM path under
# KOKORO_HOST_ROOT_DIR when running Docker sibling containers on Kokoro Instances.
function to_host_path() {
  local path="$1"
  if [ -n "${KOKORO_HOST_ROOT_DIR:-}" ] && [ -n "${KOKORO_ROOT_DIR:-}" ]; then
    local container_root="${KOKORO_ROOT_DIR%/}"
    local host_root="${KOKORO_HOST_ROOT_DIR%/}"
    if [ "${path}" = "${container_root}" ]; then
      echo "${host_root}"
    elif [[ "${path}" == "${container_root}/"* ]]; then
      echo "${host_root}${path#${container_root}}"
    else
      echo "${path}"
    fi
  else
    echo "${path}"
  fi
}

docker run --rm -i \
  --privileged \
  --volume "$(to_host_path "${ROOT_DIR}"):${ROOT_DIR}" \
  --volume "$(to_host_path "${KOKORO_ARTIFACTS_DIR}"):${KOKORO_ARTIFACTS_DIR}" \
  --workdir "${ROOT_DIR}" \
  --env ROOT_DIR="${ROOT_DIR}" \
  --env SCRIPT_DIR="${SCRIPT_DIR}" \
  --env CONFIG="${CONFIG}" \
  --env COMPILER="${COMPILER}" \
  --env SHADERC_ENABLE_HLSL="${SHADERC_ENABLE_HLSL}" \
  --env KOKORO_ARTIFACTS_DIR="${KOKORO_ARTIFACTS_DIR}" \
  --entrypoint "${SCRIPT_DIR}/build-docker.sh" \
  us-east4-docker.pkg.dev/shaderc-build/radial-docker/ubuntu-24.04-amd64/cpp-builder
