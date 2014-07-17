<?php
    require('config.php');
    require('connect.php');
    require('db.php');

    function make_page_title_map() {
        $a = Array();

        $q = "select id, title from pages";
        if (!($r = query($q)))
            return $a;

        while ($row = fetch_assoc_array($r)) {
            $id = $row["id"];
            $title = $row["title"];
            $a[$id] = $title;
        }
        free_result($r);

        return $a;
    }
    function print_header() {
        echo '<?xml version="1.0" encoding="UTF-8"?>';
        echo '<rss version="2.0">';
        echo '<channel>';
        echo '<title>S010\'s homepage</title>';
        echo '<link>myurl</link>';
    } 
    function print_items($n) {
        $page_title_map = make_page_title_map();

        $q = sprintf("select id, page_id, title, text, ctime, mtime from articles order by id desc limit %d", $n);
        if (!($r = query($q)))
            return;
        while ($row = fetch_assoc_array($r)) {
            $id = $row["id"];
            $title = htmlspecialchars($row["title"]);
            $text = $row["text"];
            $ctime = $row["ctime"];
            $mtime = $row["mtime"];
            $page_title = htmlspecialchars($page_title_map[$row["page_id"]]);
            $mtime = $row["mtime"];
            $updated_str = $ctime == $mtime ? "" : " (updated $mtime)";
            printf("<item><category>%s</category><title>[%s] %s%s</title><link>myurl/main.php?aid=%s</link><description><![CDATA[%s]]></description></item>", $page_title, $page_title, $title, $updated_str, $id, $text);
        }
        free_result($r);
    }
    function print_footer() {
        echo '</channel>';
        echo '</rss>';
    }
    function rss() {
        //header('Content-Type: text/xml'); 
        print_header();
        print_items(10);
        print_footer();
    }

    rss();

    require('disconnect.php');
?>
