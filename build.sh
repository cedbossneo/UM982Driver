#!/usr/bin/env bash
set -euo pipefail

# Build script for multi-architecture Docker images.
# Usage: ./build.sh [tag] [platforms]
# Example: ./build.sh ghcr.io/mowglifrenchtouch/um982driver:kilted linux/amd64,linux/arm64

TAG="${1:-ghcr.io/mowglifrenchtouch/um982driver:mowgli}"
PLATFORMS="${2:-linux/amd64,linux/arm64}"
BUILDER="mowgli-builder"

# Create or reuse a Buildx builder.
docker buildx inspect "$BUILDER" >/dev/null 2>&1 || docker buildx create --name "$BUILDER" --driver docker-container --bootstrap

docker buildx use "$BUILDER"

docker buildx build \
  --platform "$PLATFORMS" \
  --tag "$TAG" \
  --push \
  .
