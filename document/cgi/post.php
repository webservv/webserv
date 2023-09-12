#!/usr/bin/php
<?php
$data = fread(STDIN, $_SERVER["CONTENT_LENGTH"]);
parse_str($data, $postData);
$title = $postData["title"];
$content = $postData["content"];
$data_dir = "./DB";
$post_file = $data_dir . "/posts.txt";
$id = 0;
if (file_exists($post_file)) {
    $lines = file($post_file, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
    $id = count($lines);
}
$cookieHeader = $_SERVER["HTTP_COOKIE"];
$cookiePairs = explode(";", $cookieHeader);
$cookies = [];
foreach ($cookiePairs as $pair) {
    list($name, $value) = explode("=", trim($pair));
    $cookies[$name] = $value;
}
$SessionID = $cookies["SessionID"];
if (!empty($title) && !empty($content)) {
    $post_data = $title . "\t" . $content . "\t" . $id . "\t" . $SessionID . "\n";
    file_put_contents($post_file, $post_data, FILE_APPEND);
}

echo "Content-Type: text/html\r\n\r\n";
echo "<!DOCTYPE html>\n";
echo "<html lang='en'>\n";
echo "<head>\n";
echo "    <meta charset='UTF-8'>\n";
echo "    <meta name='viewport' content='width=device-width, initial-scale=1.0'>\n";
echo "    <title>My Website</title>\n";
echo "</head>\n";
echo "<body>\n";
echo "    <header>\n";
echo "        <h1>Welcome to My Website</h1>\n";
echo "    </header>\n";
echo "    <main>\n";
echo "        <section>\n";
echo "            <h2>Create a New Post</h2>\n";
echo "            <form action='/cgi/post.php' method='post'>\n";
echo "                <label for='title'>Title:</label>\n";
echo "                <input type='text' id='title' name='title' required><br><br>\n";
echo "                <label for='content'>Content:</label>\n";
echo "                <textarea id='content' name='content' rows='4' cols='50' required></textarea><br><br>\n";
echo "                <input type='submit' value='Submit Post'>\n";
echo "            </form>\n";
echo "        </section>\n";
echo "        <section>\n";
echo "            <h2>Discussion Topics</h2>\n";
echo "            <ul>\n";
if (file_exists($post_file)) {
    $lines = file($post_file, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
    $lines = array_reverse($lines);
    foreach ($lines as $line) {
        list($title, $content, $post_id, $cookie) = explode("\t", $line);
        echo "                <p>Title: $title</p>\n";
        echo "                <p>Content: $content</p>\n";
        echo "                <button id='deleteButton$post_id' onclick='deletePost($post_id)'>Delete</button>\n";
        echo "\n";
    }
}
echo "            </ul>\n";
echo "        </section>\n";
echo "    </main>\n";
echo "    <footer>\n";
echo "        <p>&copy; 2023 My Website. All rights reserved.</p>\n";
echo "    </footer>\n";
echo "</body>\n";
echo "<script>\n";
echo "function deletePost(post_id) {\n";
echo "    if (confirm(\"Delete this post?\")) {\n";
echo "        fetch(`/cgi/delete.pl?post_id=\${post_id}`, {\n";
echo "            method: 'DELETE',\n";
echo "            headers: {\n";
echo "                'Content-Type': 'application/json'\n";
echo "            }\n";
echo "        }).then(response => {\n";
echo "            if (response.ok) {\n";
echo "                return response.text();\n";
echo "            } else {\n";
echo "                throw new Error('delete method error!');\n";
echo "            }\n";
echo "        }).then(html => {\n";
echo "            const newDoc = document.open(\"text/html\", \"replace\");\n";
echo "            newDoc.write(html);\n";
echo "            newDoc.close();\n";
echo "        }).catch(error => {\n";
echo "            console.error('Error:', error);\n";
echo "        });\n";
echo "    }\n";
echo "}\n";    
echo "</script>\n";
echo "</html>\n";
?>