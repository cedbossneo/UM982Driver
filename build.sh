#!/usr/bin/env bash
set -euo pipefail

# Build script for multi-architecture Docker images.
# Usage: ./build_docker_multiarch.sh [tag] [platforms]
# Example: ./build_docker_multiarch.sh mowgli_unicore_gnss:kilted linux/amd64,linux/arm64

TAG="${1:-mowgli_unicore_gnss:kilted}"
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
