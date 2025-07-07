import os
import urllib.request
import urllib.parse

print("Content-type: text/html\n")

# Fixed base URL since we know your server and port
base_url = "http://localhost:8080/cgi-bin/index.py"

# Get current count from query string, default to 3
query_string = os.environ.get('QUERY_STRING', '')
params = urllib.parse.parse_qs(query_string)
count = int(params.get('count', ['600'])[0])

print("<html><body>")
print(f"<p>Count: {count}</p>")

if count > 0:
    new_count = count - 1
    next_url = f"{base_url}?count={new_count}"
    print(f"<p>Calling myself with count={new_count}: {next_url}</p>")
    try:
        with urllib.request.urlopen(next_url) as response:
            content = response.read().decode()
            print("<p>Called myself successfully.</p>")
    except Exception as e:
        print(f"<p>Error calling myself: {e}</p>")
else:
    print("<p>Reached zero count, stopping recursion.</p>")

print("</body></html>")
