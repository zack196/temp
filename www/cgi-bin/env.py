#!/usr/bin/env python3
import os

print("Content-Type: text/plain")
print()  # blank line to end headers

for key, value in sorted(os.environ.items()):
    print(f"{key}={value}")
