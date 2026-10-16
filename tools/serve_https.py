#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Pourya Afshintabar
"""Serve a directory over HTTPS for testing updates on a local network.

Create the certificates with tools/make_test_certs.sh first. The device has to
trust the test CA: copy it to main/certs/server_ca.pem and select the pinned CA
in menuconfig.
"""

import argparse
import functools
import http.server
import pathlib
import ssl


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("directory", type=pathlib.Path, help="directory to serve, for example build")
    parser.add_argument("--cert", required=True, help="server certificate (PEM)")
    parser.add_argument("--key", required=True, help="server private key (PEM)")
    parser.add_argument("--port", type=int, default=8443, help="TCP port (default: 8443)")
    args = parser.parse_args()

    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.minimum_version = ssl.TLSVersion.TLSv1_2
    context.load_cert_chain(args.cert, args.key)

    handler = functools.partial(http.server.SimpleHTTPRequestHandler, directory=str(args.directory))
    with http.server.ThreadingHTTPServer(("0.0.0.0", args.port), handler) as server:
        server.socket = context.wrap_socket(server.socket, server_side=True)
        print(f"Serving {args.directory.resolve()} at https://0.0.0.0:{args.port}/ (Ctrl+C to stop)")
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            pass


if __name__ == "__main__":
    main()
