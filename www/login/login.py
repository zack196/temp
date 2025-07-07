#!/usr/bin/env python3

import os
import cgi
import uuid
import time
import sqlite3
from datetime import datetime, timezone


def init_database():
    os.makedirs(os.path.dirname("./www/login/users.db"), exist_ok=True)
    conn = sqlite3.connect("./www/login/users.db")
    conn.execute('''
        CREATE TABLE IF NOT EXISTS users (
            id TEXT PRIMARY KEY,
            username TEXT NOT NULL,
            password TEXT NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    ''')
    conn.commit()
    conn.close()


def read_file(path):
    with open(path, 'r') as f:
        return f.read()


def get_cookie_id():
    cookies = os.environ.get('HTTP_COOKIE', '')
    for part in cookies.split(';'):
        if '=' in part:
            key, val = part.strip().split('=', 1)
            if key == 'id':
                return val
    return None


def get_user_info(user_id):
    conn = sqlite3.connect("./www/login/users.db")
    row = conn.execute('SELECT username, password FROM users WHERE id = ?', (user_id,)).fetchone()
    conn.close()
    if row:
        return {"username": row[0], "password": row[1]}
    return None


def authenticate_user(form):
    username = form.getvalue("username")
    password = form.getvalue("password")
    if not username or not password:
        return None

    conn = sqlite3.connect("./www/login/users.db")
    row = conn.execute('SELECT id FROM users WHERE username = ? AND password = ?', (username, password)).fetchone()

    if row:
        user_id = row[0]
    else:
        user_id = str(uuid.uuid4())
        conn.execute('INSERT INTO users (id, username, password) VALUES (?, ?, ?)', (user_id, username, password))
        conn.commit()

    conn.close()

    expires = time.time() + (30 * 24 * 60 * 60)
    expires_str = time.strftime("%a, %d-%b-%Y %H:%M:%S GMT", time.gmtime(expires))
    print(f"Set-Cookie: id={user_id}; Expires={expires_str}; Path=/")

    return user_id


def render_login_page():
    html = read_file("./www/login/html/login.html")
    return html.replace("{current_time}", datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S"))


def render_welcome_page(user_info):
    html = read_file("./www/login/html/welcome.html")
    return html.replace("{username}", user_info["username"]).replace("{password}", user_info["password"])


def main():
    print("Content-Type: text/html; charset=utf-8")
    form = cgi.FieldStorage()
    init_database()

    user_id = None
    user_info = None

    if "username" in form and "password" in form:
        user_id = authenticate_user(form)
    else:
        user_id = get_cookie_id()

    if user_id:
        user_info = get_user_info(user_id)

    print()

    if user_info:
        print(render_welcome_page(user_info))
    else:
        print(render_login_page())


if __name__ == "__main__":
    main()

