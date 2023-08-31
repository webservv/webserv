#!/usr/bin/python
import os

data_dir = "./DB"
post_file = os.path.join(data_dir, "posts.txt")
if not os.path.exists(data_dir):
    os.makedirs(data_dir)
print("Content-Type: text/html")
print("")
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
print("            <h2>Create a New Post</h2>")
print("            <form action='/cgi/post.php' method='post' onsubmit='submitPost(event)'>")
print("                <label for='title'>Title:</label>")
print("                <input type='text' id='title' name='title' required><br><br>")
print("                <label for='content'>Content:</label>")
print("                <textarea id='content' name='content' rows='4' cols='50' required></textarea><br><br>")
print("                <input type='submit' value='Submit'>")
print("            </form>")
print("        </section>")
print("        <section>")
print("            <h2>Discussion Topics</h2>")
print("            <ul>")
if os.path.exists(post_file):
    with open(post_file, 'r') as file:
        lines = file.readlines()
        lines.reverse()
        for line in lines:
            title, content, post_id, cookie = line.strip().split('\t')
            print("<p>Title: {}</p>".format(title))
            print("<p>Content: {}</p>".format(content))
            print("<button id='deleteButton{}' onclick='deletePost({})'>Delete</button>".format(post_id, post_id))
            print("")
print("            </ul>")
print("        </section>")
print("    </main>")
print("    <footer>")
print("        <p>&copy; 2023 My Website. All rights reserved.</p>")
print("    </footer>")
print("</body>")
print('<script>')
print("function submitPost(event) {")
print("    event.preventDefault();")
print("    fetch('/cgi/post.php', {")
print("        method: 'POST',")
print("        headers: {")
print("            'Content-Type': 'application/x-www-form-urlencoded'")
print("        },")
print("        body: new URLSearchParams({")
print("            title: document.getElementById('title').value,")
print("            content: document.getElementById('content').value")
print("        })")
print("    })")
print("    .then(response => {")
print("        if (response.ok) {")
print("            location.reload();")
print("        } else {")
print("            throw new Error('POST request error!');")
print("        }")
print("    })")
print("    .catch(error => {")
print("        console.error('Error:', error);")
print("    });")
print("}")
print("")
print("function deletePost(post_id) {")
print("    if (confirm(\"Delete this post?\")) {")
print("        fetch(`/cgi/delete.pl?post_id=${post_id}`, {")
print("            method: 'DELETE',")
print("            headers: {")
print("                'Content-Type': 'application/json'")
print("            }")
print("        })")
print("        .then(response => {")
print("            if (response.ok) {")
print("                location.reload();")
print("            } else {")
print("                throw new Error('DELETE request error!');")
print("            }")
print("        })")
print("        .catch(error => {")
print("            console.error('Error:', error);")
print("        });")
print("    }")
print("}")
print('</script>')
print("</html>")