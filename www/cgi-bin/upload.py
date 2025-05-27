#!/usr/bin/env python3

import cgi
import os

UPLOAD_DIR = '/home/mregrag/goinfre/upload/'

def print_headers():
    print("Content-Type: text/html\n")

def save_uploaded_file(file_item):
    os.makedirs(UPLOAD_DIR, exist_ok=True)

    filename = os.path.basename(file_item.filename).replace(" ", "_")

    file_path = os.path.join(UPLOAD_DIR, filename)

    with open(file_path, 'wb') as file:
        file.write(file_item.file.read())

    return filename

def handle_upload():
    form = cgi.FieldStorage()

    if 'file' not in form:
        print("<h1>Error: No file field found in the form.</h1>")
        return

    file_item = form['file']

    if not file_item.filename:
        print("<h1>No file was uploaded.</h1>")
        return

    try:
        filename = save_uploaded_file(file_item)
        print(f"<h1>File '{filename}' uploaded successfully!</h1>")
    except Exception as e:
        print(f"<h1>Error: Unable to save the file. {e}</h1>")

def main():
    print_headers()
    handle_upload()

if __name__ == "__main__":
    main()
