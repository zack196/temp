#!/usr/bin/env python3
import os, http.cookies
c = http.cookies.SimpleCookie(os.environ.get("HTTP_COOKIE", ""))
n = int(c.get("n", "0").value) + 1
c["n"] = n
c["n"]["path"] = "/"
print(c.output(header=""))
print("Content-Type: text/plain\n")
print("n =", n)
