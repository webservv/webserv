#!/usr/bin/php
<?php
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    echo "REQUEST_METHOD: POST\n";
    // Content-Type을 확인할 때 multipart/form-data를 포함하는지 여부를 검사합니다.
    if (strpos($_SERVER['CONTENT_TYPE'], 'multipart/form-data') !== false) {
        echo "CONTENT_TYPE: multipart/form-data\n";
        
        // 파일 업로드를 처리하기 위해 $_FILES 슈퍼 글로벌 배열을 사용합니다.
        if (isset($_FILES['image'])) {
            $file_name = $_FILES['image']['name'];
            echo "Uploaded File Name: $file_name\n";
        }
        else
            echo 'No $_FILES\n';
    } else {
        echo "CONTENT_TYPE: NONE\n";
    }
    // $_POST 배열을 확인하여 텍스트 필드 값을 얻습니다.
    if (isset($_POST["title"])) {
        $title = $_POST["title"];
        echo "Title: $title\n";
    }
    if (isset($_POST["content"])) {
        $content = $_POST["content"];
        echo "Content: $content\n";
    }
    
    // $_POST 배열의 크기를 확인합니다.
    $post_data_count = count($_POST);
    echo "POST size: " . $post_data_count . "\n";
    
} else {
    echo "REQUEST_METHOD: NOT POST\n";
}
// stdin을 읽어와서 출력합니다.
$data = fread(STDIN, $_SERVER["CONTENT_LENGTH"]);
if ($data !== false) {
    echo "################################\n" . $data;
}
else {
    echo "Failed to read data from stdin.\n";
    $title = "";
    $content = "";
}
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
