#!/bin/bash
set -e

# Script to create a new release
# Usage: ./scripts/create_release.sh 1.0.1

if [ $# -eq 0 ]; then
    echo "Usage: $0 <version>"
    echo "Example: $0 1.0.1"
    echo ""
    echo "This script will:"
    echo "1. Update version in debian/DEBIAN/control"
    echo "2. Update CHANGELOG.md"
    echo "3. Create and push a git tag"
    echo "4. Trigger GitHub Actions to build and release"
    exit 1
fi

VERSION=$1
TAG="v$VERSION"

echo "Creating release $TAG"

# Check if we're in the right directory
if [ ! -f "debian/DEBIAN/control" ]; then
    echo "Error: debian/DEBIAN/control not found. Run this script from the project root."
    exit 1
fi

# Check if git is clean
if [ -n "$(git status --porcelain)" ]; then
    echo "Error: Git working directory is not clean. Please commit or stash changes first."
    git status --short
    exit 1
fi

# Check if tag already exists
if git tag -l | grep -q "^$TAG$"; then
    echo "Error: Tag $TAG already exists"
    exit 1
fi

# Update version in debian control file
echo "Updating version in debian/DEBIAN/control to $VERSION"
sed -i "s/^Version: .*/Version: $VERSION/" debian/DEBIAN/control

# Update CHANGELOG.md if it exists
if [ -f "CHANGELOG.md" ]; then
    echo "Updating CHANGELOG.md"
    # Add new unreleased section
    sed -i "/^## \[Unreleased\]/a\\
\\
### Added\\
- Nothing yet\\
\\
### Changed\\
- Nothing yet\\
\\
### Fixed\\
- Nothing yet\\
\\
## [$VERSION] - $(date +%Y-%m-%d)" CHANGELOG.md
fi

# Show changes
echo "Changes to be committed:"
git diff --name-only

# Commit changes
echo "Committing version bump"
git add debian/DEBIAN/control
[ -f "CHANGELOG.md" ] && git add CHANGELOG.md
git commit -m "chore: bump version to $VERSION"

# Create tag
echo "Creating tag $TAG"
git tag -a "$TAG" -m "Release $TAG"

echo ""
echo "Release $TAG prepared successfully!"
echo ""
echo "Summary of changes:"
echo "   - Version updated to $VERSION in debian/DEBIAN/control"
echo "   - Changes committed"
echo "   - Tag $TAG created"
echo ""
echo "To complete the release, push the changes and tag:"
echo "   git push origin main"
echo "   git push origin $TAG"
echo ""
echo "After pushing, GitHub Actions will automatically:"
echo "   1. Build the .deb package"
echo "   2. Create a GitHub release"
echo "   3. Upload the package as a release asset"
echo ""
echo "Monitor the build at:"
echo "   https://github.com/dexter-xD/TinyRequest/actions"