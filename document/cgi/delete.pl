#!/usr/bin/perl
use strict;
use warnings;

my $request_method = $ENV{'REQUEST_METHOD'};
my $query_string = $ENV{'QUERY_STRING'};
my $data_dir = "./DB";
my $post_file = "$data_dir/posts.txt";

if ($request_method eq 'DELETE' && $query_string =~ /id=(\d+)/) {
    my $idToDelete = $1;

    if (-e $post_file) {
        open my $fh, '<', $post_file or die "Failed to open $post_file: $!";
        my @newLines;

        while (my $line = <$fh>) {
            my ($title, $content, $id, $sessionID) = split("\t", $line);

            if ($id != $idToDelete) {
                push @newLines, $line;
            }
        }

        close $fh;
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
        print("                    <button id='deleteButton$post_id' onclick='deletePost($post_id)'>Delete</button>");
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
print "<script>\n";
print "function deletePost(post_id) {\n";
print "    if (confirm(\"Delete this post?\")) {\n";
print "        fetch(`/cgi/delete.pl?post_id=\${post_id}`, {\n";
print "            method: 'DELETE',\n";
print "            headers: {\n";
print "                'Content-Type': 'application/json'\n";
print "            }\n";
print "        }).then(response => {\n";
print "            if (response.ok) {\n";
print "                return response.text();\n";
print "            } else {\n";
print "                throw new Error('delete method error!');\n";
print "            }\n";
print "        }).then(html => {\n";
print "            const newDoc = document.open(\"text/html\", \"replace\");\n";
print "            newDoc.write(html);\n";
print "            newDoc.close();\n";
print "        }).catch(error => {\n";
print "            console.error('Error:', error);\n";
print "        });\n";
print "    }\n";
print "}\n";
print "</script>\n";
print("</html>\n");