#~/vcpkg/vcpkg install  --triplet x64-linux --overlay-triplets=./triplets --overlay-ports=./ports-overlay --recurse --no-binarycaching
export VCPKG_MAX_CONCURRENCY=6
#~/vcpkg/vcpkg install  --triplet x64-linux --overlay-triplets=./triplets --overlay-ports=./ports-overlay 
~/vcpkg/vcpkg install  --triplet x64-linux --overlay-triplets=./triplets
