#!/usr/bin/php

<?php
$data = fread(STDIN, $_SERVER["CONTENT_LENGTH"]);
if ($data !== false) {
    parse_str($data, $postData);
    $title = $postData["title"];
    $content = $postData["content"];
}
else {
    echo "Failed to read data from stdin.\n";
    $title = "";
    $content = "";
}

echo "<!DOCTYPE html>";
echo "<html lang='en'>";
echo "<head>";
echo "    <meta charset='UTF-8'>";
echo "    <meta name='viewport' content='width=device-width, initial-scale=1.0'>";
echo "    <title>My Website</title>";
echo "</head>";
echo "<body>";
echo "    <header>";
echo "        <h1>Welcome to My Website</h1>";
echo "    </header>";
echo "    <main>";
echo "        <section>";
echo "            <h2>Discussion Topics</h2>";
if ($title && $content) {
    echo "            <p>Title: " . htmlspecialchars($title) . "</p>";
    echo "            <p>Content: " . htmlspecialchars($content) . "</p>";
} else {
    echo "            <p>No data received.</p>";
}
echo "        <section>";
echo "            <h2>Create a New Post</h2>";
echo "            <form action='/cgi/post.php' method='post'>";
echo "                <label for='title'>Title:</label>";
echo "                <input type='text' id='title' name='title' required><br><br>";
echo "                ";
echo "                <label for='content'>Content:</label>";
echo "                <textarea id='content' name='content' rows='4' cols='50' required></textarea><br><br>";
echo "                ";
echo "                <input type='submit' value='Submit Post'>";
echo "            </form>";
echo "        </section>";
echo "        </section>";
echo "    </main>";
echo "    <footer>";
echo "        <p>&copy; 2023 My Website. All rights reserved.</p>";
echo "    </footer>";
echo "</body>";
echo "</html>";
?>
