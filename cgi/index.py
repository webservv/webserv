#!/usr/bin/python

print("Content-Type: text/html")
print()
print("""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>My Website</title>
</head>
<body>
    <header>
        <h1>Welcome to My Website</h1>
    </header>
    <main>
        <section>
            <h2>Discussion Topics</h2>
        </section>

        <section>
            <h2>Create a New Post</h2>
            <form action="/cgi/post.php" method="post">
                <label for="title">Title:</label>
                <input type="text" id="title" name="title" required><br><br>

                <label for="content">Content:</label>
                <textarea id="content" name="content" rows="4" cols="50" required></textarea><br><br>

                <input type="submit" value="Submit Post">
            </form>
        </section>
    </main>
    
    <footer>
        <p>&copy; 2023 My Website. All rights reserved.</p>
    </footer>
</body>
</html>
""")