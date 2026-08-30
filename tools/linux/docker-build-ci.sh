REPO_ROOT="$(git rev-parse --show-toplevel)"
cd "$REPO_ROOT" || exit 1
docker build --progress=plain -t ghcr.io/xcyxiner/dicomviewer-ci:latest . 2>&1 | tee build.log