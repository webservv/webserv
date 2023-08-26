#!/usr/bin/perl
use strict;
use warnings;

# HTTP 요청 메서드와 환경 변수를 읽어옴
my $request_method = $ENV{'REQUEST_METHOD'};
my $query_string = $ENV{'QUERY_STRING'};

# 데이터 디렉토리와 게시물 파일 경로 설정
my $data_dir = "./DB";
my $post_file = "$data_dir/posts.txt";

if ($request_method eq 'DELETE' && $query_string =~ /id=(\d+)/) {
    my $idToDelete = $1;

    if (-e $post_file) {
        open my $fh, '<', $post_file or die "Failed to open $post_file: $!";
        my @newLines;

        while (my $line = <$fh>) {
            my ($title, $content, $id, $sessionID) = split("\t", $line);

            # 삭제하려는 id와 일치하지 않는 항목만 새로운 배열에 추가
            if ($id != $idToDelete) {
                push @newLines, $line;
            }
        }

        close $fh;

        # 파일에 변경된 내용을 저장
        open my $out_fh, '>', $post_file or die "Failed to open $post_file: $!";
        print $out_fh @newLines;
        close $out_fh;
    }
}

print "Content-Type: text/html\r\n\r\n";
print("<!DOCTYPE html>\n");
print("<html lang='en'>\n");
print("<head>\n");
print("    <meta charset='UTF-8'>\n");
print("    <meta name='viewport' content='width=device-width, initial-scale=1.0'>\n");
print("    <title>My Website</title>\n");
print("</head>\n");
print("<body>\n");
print("    <header>\n");
print("        <h1>Welcome to My Website</h1>\n");
print("    </header>\n");
print("    <main>\n");
print("        <section>\n");
print("            <h2>Create a New Post</h2>\n");
print("            <form action='/cgi/post.php' method='post'>\n");
print("                <label for='title'>Title:</label>\n");
print("                <input type='text' id='title' name='title' required><br><br>\n");
print("                <label for='content'>Content:</label>\n");
print("                <textarea id='content' name='content' rows='4' cols='50' required></textarea><br><br>\n");
print("                <input type='submit' value='Submit Post'>\n");
print("            </form>\n");
print("        </section>\n");
print("        <section>\n");
print("            <h2>Discussion Topics</h2>\n");
print("            <ul>\n");
if (-e $post_file) {
    open my $file, '<', $post_file or die "Cannot open file: $!";
    my @lines = reverse <$file>;
    close $file;
    
    foreach my $line (@lines) {
        my ($title, $content, $post_id, $cookie) = split("\t", $line);
        print("                <li>\n");
        print("                    <p>Title: $title</p>\n");
        print("                    <p>Content: $content</p>\n");
        # print("                    <button id='deleteButton$post_id' onclick='deletePost($post_id)'>Delete</button>");
        print("                </li>\n");
    }
}
print("            </ul>\n");
print("        </section>\n");
print("    </main>\n");
print("    <footer>\n");
print("        <p>&copy; 2023 My Website. All rights reserved.</p>\n");
print("    </footer>\n");
print("</body>\n");
# print("<script>\n");
# print("function deletePost(post_id) {\n");
# print("    if (confirm(\"Delete this post?\")) {\n");
# print("        fetch(`/cgi/delete.pl?post_id=post_id`, {\n");
# print("            method: 'DELETE',\n");
# print("            headers: {\n");
# print("                'Content-Type': 'application/json'\n");
# print("            }\n");
# print("        })\n");
# print("    }\n");
# print("}\n");
print("</script>\n");
print("</html>\n");