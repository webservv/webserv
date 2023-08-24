#!/usr/bin/python

import cgi

form = cgi.FieldStorage()
title = form.getvalue("title")
content = form.getvalue("content")

print("Content-type: text/html\n")

print("<!DOCTYPE html>")
print("<html lang='en'>")
print("<head>")
print("    <meta charset='UTF-8'>")
print("    <meta name='viewport' content='width=device-width, initial-scale=1.0'>")
print("    <title>My Website</title>")
print("</head>")
print("<body>")
print("    <header>")
print("        <h1>Welcome to My Website</h1>")
print("    </header>")
print("    <main>")
print("        <section>")
print("            <h2>Discussion Topics</h2>")
if title and content:
    print("            <p>Title: {}</p>".format(title))
    print("            <p>Content: {}</p>".format(content))
else:
    print("            <p>No data received.</p>")
print("        </section>")
print("    </main>")
print("    <footer>")
print("        <p>&copy; 2023 My Website. All rights reserved.</p>")
print("    </footer>")
print("</body>")
print("</html>")