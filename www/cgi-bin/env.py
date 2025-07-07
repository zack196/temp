#!/usr/bin/python3
import os
import sys
import cgi
import time
import datetime
print("<!DOCTYPE html>")
print("<html>")
print("<head>")
print("<title>CGI Test Script</title>")
print("<style>")
print("body { font-family: Arial, sans-serif; margin: 20px; }")
print("h1 { color: #2c3e50; }")
print("table { border-collapse: collapse; width: 100%; }")
print("th, td { text-align: left; padding: 8px; border: 1px solid #ddd; }")
print("tr:nth-child(even) { background-color: #f2f2f2; }")
print("th { background-color: #4CAF50; color: white; }")
print("</style>")
print("</head>")
print("<body>")

print("<h2>Environment Variables</h2>")
print("<table>")
print("<tr><th>Variable</th><th>Value</th></tr>")

env_vars = sorted(os.environ.items())
for key, value in env_vars:
    print("<tr><td>{}</td><td>{}</td></tr>".format(key, value))
print("</table>")

# End HTML
print("</body>")
print("</html>")
