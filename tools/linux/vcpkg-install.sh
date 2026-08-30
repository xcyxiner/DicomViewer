#~/vcpkg/vcpkg install  --triplet x64-linux --overlay-triplets=./triplets --overlay-ports=./ports-overlay --recurse --no-binarycaching
export VCPKG_MAX_CONCURRENCY=6
#~/vcpkg/vcpkg install  --triplet x64-linux --overlay-triplets=./triplets --overlay-ports=./ports-overlay 
REPO_ROOT="$(git rev-parse --show-toplevel)"
cd "$REPO_ROOT" || exit 1
~/vcpkg/vcpkg install --x-install-root=build --triplet x64-linux --overlay-triplets=./triplets
