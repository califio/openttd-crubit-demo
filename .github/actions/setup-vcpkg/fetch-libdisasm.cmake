# SourceForge's download endpoint returns an HTML page on the CI runners.
# Debian mirrors the identical upstream archive. Keep vcpkg's original SHA-512;
# vcpkg also verifies the file when consuming it from its download cache.
file(TO_CMAKE_PATH "$ENV{VCPKG_DOWNLOADS}" downloads)
file(DOWNLOAD
    "https://deb.debian.org/debian/pool/main/libd/libdisasm/libdisasm_0.23.orig.tar.gz"
    "${downloads}/libdisasm-0.23.tar.gz"
    EXPECTED_HASH SHA512=29eecfbfd8168188242278a1a38f0c90770d0581a52d4600ae6343829dd0d6607b98329f12a3d7409d43dd56dca6a7d1eb25d58a001c2bfd3eb8474c0e7879e7
    TLS_VERIFY ON
    TIMEOUT 120)
