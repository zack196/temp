#!/usr/bin/env python3
import os, http.cookies

c = http.cookies.SimpleCookie(os.environ.get("HTTP_COOKIE", ""))

if "hits" in c:
    hits = int(c["hits"].value) + 1
else:
    hits = 1

c["hits"] = hits
c["hits"]["path"] = "/"
c["hits"]["max-age"] = 3600

print(c.output(header=""))
print("Content-Type: text/plain")
print()
print(f"visit #{hits}\n")
