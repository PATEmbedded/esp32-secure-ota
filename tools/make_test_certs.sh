#!/usr/bin/env bash
# Creates a throwaway CA and a server certificate for testing updates on a
# local network. Never use these files in production.
#
# Usage: tools/make_test_certs.sh <address of the PC that serves the image> [output directory]
#   tools/make_test_certs.sh 192.168.1.20
#
# Writes ca.pem, ca.key, server.pem and server.key to test-certs/ (ignored by git).
set -euo pipefail

# Git for Windows would rewrite "/CN=..." into a file path; no effect elsewhere.
export MSYS_NO_PATHCONV=1

host="${1:?usage: $0 <server IP address or host name> [output directory]}"
out="${2:-test-certs}"
mkdir -p "$out"

# mbedTLS matches the host name as text, so an IP address goes into both SAN types.
san="DNS:${host}"
if [[ "$host" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    san="${san},IP:${host}"
fi

# Plain files instead of process substitution, so the script also works with
# the native OpenSSL of Git for Windows.
cat > "$out/openssl.cnf" <<'EOF'
[req]
distinguished_name = dn
[dn]
[ca]
basicConstraints = critical, CA:TRUE
keyUsage = critical, keyCertSign, cRLSign
subjectKeyIdentifier = hash
EOF
printf 'basicConstraints = CA:FALSE\nextendedKeyUsage = serverAuth\nsubjectAltName = %s\n' "$san" > "$out/server.ext"

openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -nodes -days 30 \
    -subj "/CN=esp32-secure-ota test CA" -config "$out/openssl.cnf" -extensions ca \
    -keyout "$out/ca.key" -out "$out/ca.pem"

openssl req -new -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -nodes \
    -subj "/CN=${host}" -config "$out/openssl.cnf" \
    -keyout "$out/server.key" -out "$out/server.csr"

openssl x509 -req -in "$out/server.csr" -CA "$out/ca.pem" -CAkey "$out/ca.key" -CAcreateserial -days 30 \
    -extfile "$out/server.ext" -out "$out/server.pem"

rm -f "$out/openssl.cnf" "$out/server.ext" "$out/server.csr" "$out/ca.srl"
echo "Created $out/. Copy $out/ca.pem to main/certs/server_ca.pem and select the pinned CA in menuconfig."
