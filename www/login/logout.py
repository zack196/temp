#!/usr/bin/python3

import os
import cgitb

cgitb.enable()

def clear_cookie():
    # Clear the cookie first
    print("Set-Cookie: id=; expires=Thu, 01 Jan 1970 00:00:00 GMT; Path=/")

    # Send redirect headers
    print("Status: 302 Found")
    print("Location: /login")
    print("Content-Type: text/html\r\n")

    # Print minimal content in case redirect fails
    print("""
    <!DOCTYPE html>
    <html>
    <head>
        <meta http-equiv="refresh" content="0;url=/login">
    </head>
    </html>
    """)

if __name__ == "__main__":
    try:
        clear_cookie()
    except Exception as e:
        print("Content-Type: text/html\r\n")
        print(f"<html><body><h1>Error: {str(e)}</h1></body></html>")
