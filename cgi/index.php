#!/usr/bin/php
<?php
print("Content-Type: text/html");
echo "\r\n\r\n";

echo <<<HTML
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
    
    <nav>
        <ul>
            <li><a href="#">Home</a></li>
            <li><a href="#">About</a></li>
            <li><a href="#">Services</a></li>
            <li><a href="#">Contact</a></li>
        </ul>
    </nav>
    
    <main>
        <section>
            <h2>About Us</h2>
            <p>Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed at felis sed ligula dictum vulputate.</p>
        </section>
        
        <section>
            <h2>Our Services</h2>
            <ul>
                <li>Web Development</li>
                <li>Graphic Design</li>
                <li>Digital Marketing</li>
            </ul>
        </section>

        <section>
            <h2>Discussion Topics</h2>
        </section>

        <section>
            <h2>Create a New Post</h2>
            <form action="/cgi/post.py" method="post">
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
HTML;
?>
