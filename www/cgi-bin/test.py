def main():
    # Print HTTP headers
    print("Content-Type: text/html")  # Header to specify the content type
    print("X-Custom-Header: TestHeader")  # Custom header
    print("")  # Blank line to separate headers from the body

    # Print the body
    print("<html>")
    print("<head><title>CGI Response</title></head>")
    print("<body>")
    print("<h1>This is a CGI Response</h1>")
    print("<p>This response includes both headers and a body.</p>")
    print("<p>Use this script to test your CGI handling logic.</p>")
    print("</body>")
    print("</html>")

if __name__ == "__main__":
    main()
