<?php
    error_reporting(E_ALL);

    session_start();

    require("config.php");
    require("connect.php");

    require('db.php');
    require("add_article.php");

    ++$articles_per_page; // select one more, to see if we need to show next page link


    define("GUEST", 0);
    define("USER", 1);
    define("ADMIN", 2);

    if (isset($_SESSION["priv_level"])) {
        $login = $_SESSION["login"];
        $priv_level = $_SESSION["priv_level"];
    } else {
        $login = "Guest";
        $priv_level = GUEST;
    }

    $get_action = isset($_GET["action"]) ? $_GET["action"] : "";
    $post_action = isset($_POST["action"]) ? $_POST["action"] : "";

    if ($get_action == "logout") {
        session_destroy();
        $priv_level = GUEST;
        $login = "Guest";
    } else if ($post_action == "login") {
        perform_login();
    }

    /* convert page OFFSET into the ID in db */
    function page2pid($page) {
        $q = "select id from pages order by priority asc limit 1 offset $page";
        $r = query($q);
        if (!($row = pg_fetch_array($r, null, PGSQL_ASSOC))) {
            pg_free_result($r);
            return 0;
        }
        $pid = $row["id"];
        pg_free_result($r);
        return $pid;
    }
    function perform_login() {
        global $login, $priv_level;

        $login = substr($_POST["login"], 0, 256);
        $password = $_POST["password"];

        $pat = "/[^a-zA-Z0-9_ ]/";

        $login = preg_replace($pat, "_", $login);
        $password = preg_replace($pat, "_", $password);

        $q = "select login, class from users where login = '" . $login . "' and password = '" . $password . "' limit 1";
        $results = query($q);
        $row = pg_fetch_array($results, null, PGSQL_ASSOC);
        if ($row) {
            print_r($row);

            $class = $row["class"];

            if ($class == "A") {
                $priv_level = ADMIN;
                $_SESSION["login"] = $login;
                $_SESSION["priv_level"] = $priv_level;
            } else if ($class = "U") {
                $priv_level = USER;
                $_SESSION["login"] = $login;
                $_SESSION["priv_level"] = $priv_level;
            }
        } else {
            $login = "Guest";
            //error_log("perform_login: results empty");
        }
        pg_free_result($results);
    }
    function perform_logout() {
        global $login, $priv_level;

        session_destroy();
        $login = "Guest";
        $priv_level = GUEST;
    }
    function display_login_status() {
        global $login, $priv_level;
        
        if ($priv_level == GUEST)
            return;
        echo "<div class=\"login_status\">Logged in as $login. <a href=\"main.php?action=logout\">Click here to logout.</a></div>\n";
    }
    function tagstr2array($s) {
        $a = explode(',', $s);
        for ($i = 0; $i < sizeof($a); ++$i)
            $a[$i] = strtolower(trim($a[$i]));
        return $a;
    }
    function get_last_post_on_page($pid) {
        $q = "select id from articles where page_id = $pid order by id desc";
        if (!($r = query($q))) {
            pg_free_result($r);
            return false;
        }
        if (!($row = pg_fetch_array($r, null, PGSQL_ASSOC))) {
            pg_free_result($r);
            return false;
        }
        $aid = $row["id"];
        pg_free_result($r);
        return $aid;
    } 
    function update_article_tags($aid, $tags) {
        $q = "delete from tags where article_id = $aid";
        $r = query($q);
        pg_free_result($r);

        for ($i = 0; $i < sizeof($tags); ++$i) {
            if ($tags[$i] == "")
                continue;
            $q = "insert into tags (article_id, tag) values ($aid, '" . $tags[$i] . "')";
            $r = query($q);
            pg_free_result($r);
        }
    }
    function add_article() {
        global $page, $crossposters;

        if ($page < 0)
            return;

        $pid = page2pid($page);

        if (!isset($_POST["text"]))
            return;
        $title = isset($_POST["title"]) ? html_entity_decode($_POST["title"], ENT_QUOTES, "UTF-8") : "";
        $tag_str = isset($_POST["tags"]) ? htmlspecialchars(html_entity_decode($_POST["tags"], ENT_QUOTES, "UTF-8"), ENT_QUOTES) : "";
        $do_crosspost = intval(get_val($_POST, "do_crosspost", "0"));
        $tags = tagstr2array($tag_str);
        //error_log("tag_str: $tag_str");
        $text = html_entity_decode($_POST["text"], ENT_QUOTES, "UTF-8");
        $ctime = date("Y-m-d H:i:s");

        if ($do_crosspost) {
            foreach ($crossposters as $crossposter) {
                if (!$crossposter["enabled"])
                    continue;
                $f = fopen("__tmp__", "w");
                fwrite($f, $text, strlen($text));
                fwrite($f, $crossposter["comment"], strlen($crossposter["comment"]));
                fclose($f);
                $exec_str = sprintf($crossposter["prog_fmt"], $crossposter["username"], $crossposter["password"], $title, "__tmp__");
                exec($exec_str);
                unlink("__tmp__");
            }
        }

        $ra = array(
            "/\n/", "\\n",
            "/\\\\/", "\\\\",
            "/'/", "\\'"
        );
        for ($i = 0; $i < sizeof($ra); $i += 2) {
            $pat = $ra[$i];
            $rep = $ra[$i+1];
            $title = preg_replace($pat, $rep, $title);
            $text = preg_replace($pat, $rep, $text);
        }
            
        $q = "insert into articles (title, text, page_id, ctime, mtime) values ('$title', '$text', $pid, '$ctime', '$ctime')";
        $r = query($q);
        pg_free_result($r);

        if (!($aid = get_last_post_on_page($pid)))
            return;
        update_article_tags($aid, $tags);
    }
    function edit_article() {
        if (!isset($_POST["text"]) || !isset($_POST["aid"]))
            return;
        if (($aid = intval($_POST["aid"])) < 0)
            return;
        $title = isset($_POST["title"]) ? html_entity_decode($_POST["title"], ENT_QUOTES, "UTF-8") : "";
        $text = html_entity_decode($_POST["text"], ENT_QUOTES, "UTF-8");
        $tag_str = isset($_POST["tags"]) ? htmlspecialchars(html_entity_decode($_POST["tags"], ENT_QUOTES, "UTF-8"), ENT_QUOTES) : "";
        $tags = tagstr2array($tag_str);
        $mtime = date("Y-m-d H:i:s");

        $ra = array(
            "/\\\\/", "\\\\",
            "/'/", "\'"
        );
        for ($i = 0; $i < sizeof($ra); $i += 2) {
            $pat = $ra[$i];
            $rep = $ra[$i+1];
            $title = preg_replace($pat, $rep, $title);
            $text = preg_replace($pat, $rep, $text);
        }

        $q = "update articles set title = '$title', text = '$text', mtime = '$mtime' where id = $aid";
        $r = query($q);
        pg_free_result($r);
        update_article_tags($aid, $tags);
    }
    function delete_article($aid) {
        global $pid;

        if ($pid == -1)
            return;

        $q = array(
            "delete from comments where article_id = $aid",
            "delete from tags where article_id = $aid",
            "delete from articles where id = $aid"
        );
        for ($i = 0; $i < sizeof($q); ++$i) {
            $r = query($q[$i]);
            pg_free_result($r);
        }
    }
    function display_menu() {
        global $priv_level, $page;

        echo "<p>";
        echo "<div class=\"menu\">\n";

        $q = "select id, title, descr from pages order by priority asc";
        $results = query($q);
        $i = 0;
        while ($row = pg_fetch_array($results, null, PGSQL_ASSOC)) {
            $id = $row["id"];
            $title = $row["title"];
            if (($descr = $row["descr"]) == "")
                $descr = "&nbsp;";
            $class = $i == $page ? "current_page_link" : "page_link";
            
            echo "<a class=\"$class\" href=\"main.php?page=$i\" onmouseover=\"set_page_descr('$descr');\" onmouseout=\"set_page_descr('&nbsp;');\">" . $title . "</a> ";
            if ($priv_level == ADMIN) {
                echo "<a href=\"main.php?action=delete_page&page=$id\" onclick=\"return confirm('Are you sure you want to delete a page with all it\'s articles and comments?');\">Delete page</a>";
            }
            ++$i;
        }
        echo "<a href=\"rss.php\"><span class=\"rss_icon\"><img src=\"feed-icon-14x14.png\"></span></a>\n";
        echo "</div>\n";
        echo "<div class=\"page_descr\" id=\"div_page_descr\">&nbsp;</div>\n";
        echo "</p>";
        pg_free_result($results);
    }
    function display_rss_icon() {
        echo "<div style=\"width: 100%; z-index: 100; position: fixed\" align=\"right\"><a href=\"rss.php\"><img src=\"feed-icon-28x28.png\"></a></div>\n";
    }
    function display_article_admin_ctrls($id) {
        echo "<p class=\"article_admin_ctrls\">";
        echo "<a href=\"main.php?action=del_article&aid=$id\" onclick=\"return confirm('Are you sure you want to delete this article with all it\'s comments?');\">Delete</a> ";
        echo "<a href=\"main.php?action=edit_article&aid=$id\">Edit</a>";
        echo "</p>\n";
    }
    function display_comment($id, $title, $text, $name, $website, $email) {
        global $priv_level;

        if ($name == "")
            $name = "Anonymous";

        echo "<p>\n";
        echo "<table class=\"comment\" border=\"0\">\n";
        echo "<tr class=\"comment_name\"><td class=\"comment_name\">$name</td></tr>\n";
        if ($priv_level == ADMIN) {
            echo "<tr class=\"comment_email\"><td class=\"comment_email\"><a href=\"mailto:$email\">$email</a></td></tr>\n";
        }
        echo "<tr class=\"comment_website\"><td class=\"comment_website\">$website</td></tr>\n";
        echo "<tr class=\"comment_title\"><td class=\"comment_title\">$title</td></tr>\n";
        echo "<tr class=\"comment_text\"><td class=\"comment_text\">$text</td></tr>\n";
        if ($priv_level == ADMIN) {
            echo "<tr><td colspan=\"2\"><a href=\"main.php?action=del_comment&cid=$id\">Delete</a> <a href=\"main.php?action=ban_ip&cid=$id\">Ban IP</a></td></tr>";
        }
        echo "</table>\n";
        echo "</p>\n";
    }
    function display_article_comments($aid) {
        $q = "select id, title, text, name, website, email from comments where article_id = $aid order by id asc";
        if (!($r = query($q))) {
            pg_free_result($r);
            return;
        }
        while ($row = pg_fetch_array($r, null, PGSQL_ASSOC)) {
            display_comment($row["id"], $row["title"], $row["text"], $row["name"], $row["website"], $row["email"]);
        }
        pg_free_result($r);
    }
    function get_article_tags($aid) {
        $q = "select tag from tags where article_id = $aid order by tag asc";
        $r = query($q);
        $i = 0;
        $a = array();
        while ($row = pg_fetch_array($r, null, PGSQL_ASSOC)) {
            $a[$i++] = $row["tag"];
        }
        pg_free_result($r);
        return $a;
    }
    function display_single_article($id, $title, $tags, $text, $ctime, $mtime) {
        global $priv_level, $page;

        echo "<h2><a class=\"article_link\" href=\"main.php?aid=$id\">$title</a></h2>\n";
        echo "<table class=\"article_time\">\n<tr><td>ctime:</td><td>$ctime</td></tr>\n<tr><td>mtime:</td><td>$mtime</td></tr></table>\n";
        if (sizeof($tags)) {
            echo "<span class=\"article_tags\">Entry tags: ";
            for ($i = 0; $i < sizeof($tags); ++$i) {
                if ($i != 0)
                    echo ", ";
                $tag = $tags[$i];
                echo "<a class=\"article_tag_link\" href=\"main.php?tag=$tag&page=$page\">$tag</a>";
            }
            echo "</span>\n";
        }
        echo "<p><div class=\"article_text\">$text</div></p>\n";
        if ($priv_level == ADMIN)
            display_article_admin_ctrls($id);
    }
    function display_next_prev_page_links($tag, $disp_next_page_link = true) {
        global $page, $articles_per_page, $skip;

        $tag_str = $tag ? "&tag=$tag" : "";

        echo "<p class=\"next_prev_page_links\">";
        if ($skip > 0) {
            if (($new_skip = $skip - $articles_per_page - 1) < 0)
                $skip_str = "";
            else
                $skip_str = "&skip=$new_skip";
            echo "<a href=\"main.php?page=$page$skip_str$tag_str\">&lt;- prev. page</a> ";
        }

        if ($disp_next_page_link) {
            $new_skip = $skip + $articles_per_page - 1;
            echo "<a href=\"main.php?page=$page&skip=$new_skip$tag_str\">next page -&gt;</a>";
        }
        echo "</p>\n";
    } 
    function display_page($page, $tag, $skip) {
        global $priv_level, $articles_per_page;
        $pid = 0;

        if ($page < 0 && $tag) 
            $q = "select a.id, a.title, a.text, a.ctime, a.mtime, t.article_id, t.tag from articles a, tags t where t.tag = '$tag' and t.article_id = a.id order by a.id desc limit $articles_per_page offset $skip";
        else if ($tag) {
            $pid = page2pid($page);
            $q = "select a.id, a.title, a.text, a.ctime, a.mtime, t.article_id, t.tag from articles a, tags t where t.tag = '$tag' and t.article_id = a.id and a.page_id = $pid order by a.id desc limit $articles_per_page offset $skip";
        } else {
            if ($page < 0)
                $page = 0;
            $pid = page2pid($page);
            $q = "select id, title, text, ctime, mtime from articles where page_id = $pid order by id desc limit $articles_per_page offset $skip"; 
        }

        echo "Tags: ";
        display_tag_cloud($page);
        echo "<hr>\n";

        $r = query($q);
        $i = 0;
        $disp_next_page_link = false; 
        while ($row = pg_fetch_array($r, null, PGSQL_ASSOC)) {
            if ($i == $articles_per_page - 1) {
                $disp_next_page_link = true;
                break;
            }
            $id = $row["id"];
            $title = $row["title"];
            $text = $row["text"];
            $ctime = $row["ctime"];
            $mtime = $row["mtime"];

            echo "<div class=\"article_container\">\n";
            display_single_article($id, $title, get_article_tags($id), $text, $ctime, $mtime);
            display_article_comment_links($id);
            echo "</div>\n";
            ++$i;
        }
        pg_free_result($r);
        echo "<hr>\n";
        display_next_prev_page_links($tag, $disp_next_page_link);
    }
    function display_article_comment_links($aid) {
        $q = "select count(id) from comments where article_id = $aid";
        if (!($r = query($q)))
            return;
        $row = fetch_assoc_array($r);
        $count = intval($row['count']);
        free_result($r);
        if ($count < 1)
            return;
        if ($count & 1)
            $word = "comment";
        else
            $word = "comments";
        echo "<a href=\"main.php?aid=$aid\">$count $word</a>\n";
    }
    function gen_captcha_code($len) {
        $s = "";
        for ($i = 0; $i < $len; ++$i) {
            $t = rand(0, 2);
            if ($t == 0)
                $s .= chr(rand(ord('a'), ord('z')));
            else if ($t == 1)
                $s .= chr(rand(ord('A'), ord('Z')));
            else
                $s .= chr(rand(ord('0'), ord('9')));
        }
        return $s;
    } 
    function display_captcha($captcha) {
        echo "<pre class=\"captcha\">" . chop(shell_exec("banner $captcha")) . "</pre>\n";
    }
    function display_comment_form($aid) {
        global $captcha_length, $captcha_seed;

        if (is_banned_ip($_SERVER["REMOTE_ADDR"]))
            return;

        $captcha = gen_captcha_code($captcha_length);
        $hash = md5($captcha_seed . $captcha);
        $plain_hash = md5($captcha);

        //error_log("captcha: $captcha");
        //error_log("hash: $hash");

        echo "<form id=\"comment_form\" action=\"main.php?aid=$aid\" method=\"post\" onsubmit=\"return check_captcha();\">\n";
        echo "<table class=\"post_comment\" border=\"0\">\n";
        echo "<tr><td>Name:</td><td><input type=\"text\" name=\"name\" style=\"width: 100%\"></td></tr>\n";
        echo "<tr><td><sup>*</sup>email:</td><td><input type=\"text\" name=\"email\" style=\"width: 100%\"></td></tr>\n";
        echo "<tr><td>Website:</td><td><input type=\"text\" name=\"website\" style=\"width: 100%\"></td></tr>\n";
        echo "<tr><td>Title:</td><td><input type=\"text\" name=\"title\" style=\"width: 100%\"></td></tr>\n";
        echo "<tr><td>Text:</td><td><textarea name=\"text\" rows=\"15\" cols=\"45\"></textarea></td></tr>\n";
        echo "<tr><td></td><td>";
            display_captcha($captcha);
        echo "</td></tr>\n";
        echo "<tr><td>Enter the code:</td><td><input id=\"captcha_code\" type=\"text\" name=\"captcha\" style=\"width: 100%\"></td></tr>\n";
        echo "<tr><td colspan=\"2\"><div align=\"right\"><input type=\"submit\" value=\"Post a comment\"></div></td></tr>\n";
        echo "</table>\n";
        echo "<input type=\"hidden\" name=\"action\" value=\"post_comment\">\n";
        echo "<input type=\"hidden\" name=\"captcha_hash\" value=\"$hash\">\n";
        echo "<input type=\"hidden\" id=\"plain_captcha_hash\" value=\"$plain_hash\">\n";
        echo "</form>\n";
    }
    function display_article($aid) {
        global $priv_level;

        $q = "select id, title, text, ctime, mtime from articles where id = $aid";
        $r = query($q);
        if (!($row = pg_fetch_array($r, null, PGSQL_ASSOC))) {
            pg_free_result($r);
            return;
        }

        $id = $row["id"];
        $title = $row["title"];
        $text = $row["text"];
        $ctime = $row["ctime"];
        $mtime = $row["mtime"];
        pg_free_result($r);

        display_single_article($id, $title, get_article_tags($id), $text, $ctime, $mtime);
        echo "<hr>\n";
        display_comment_form($aid);
        echo "<hr>\n";
        display_article_comments($id);
    }
    function display_page_admin_ctrls() {
        global $page;

        echo "<a href=\"main.php?action=add_article&page=$page\">Add an article</a>";
    }
    function make_website_link($s) {
        if (preg_match("/^http:\/\//", $s) < 1)
            $s = "http://" . $s;
        if (preg_match("/\w+\.\w\w+/", $s) < 1)
            return "";
        return "<a href=\"" . preg_replace("/\"/", "&quot;", $s) . "\">" . htmlspecialchars($s) . "</a>";
    }
    function get_article_title($aid) {
        $q = "select title from articles where id = $aid";
        $r = query($q);
        if (!$r)
            return "";
        $row = fetch_assoc_array($r);
        $title = $row["title"];
        free_result($r);
        return $title;
    }
    function get_comment_fld($s) {
        $s = html_entity_decode($s, ENT_QUOTES, "UTF-8");
        $s = htmlspecialchars($s, ENT_QUOTES);
        return $s;
    }
    function get_comment_form_fld($fld_name, $default_value = "") {
        $fld = get_val($_POST, $fld_name, $default_value);
        return get_comment_fld($fld);
    }
    function post_comment() {
        global $aid, $captcha_length, $captcha_seed, $email_notification;

        $ip = $_SERVER["REMOTE_ADDR"];

        if (is_banned_ip($ip))
            return;

        //$name = isset($_POST["name"]) ? get_comment_fld($_POST["name"], ENT_QUOTES) : "Anonymous";
        //$email = isset($_POST["email"]) ? get_comment_fld($_POST["email"], ENT_QUOTES) : "";
        //$website = isset($_POST["website"]) ? make_website_link($_POST["website"]) : "";
        //$title = isset($_POST["title"]) ? get_comment_fld($_POST["title"], ENT_QUOTES) : "";
        $name = get_comment_form_fld("name", "Anonymous");
        $email = get_comment_form_fld("email");
        $website = get_comment_form_fld("website");
        $title = get_comment_form_fld("title");
        if (!isset($_POST["text"]))
            return;
        $text = get_comment_fld($_POST["text"]);

        if (!isset($_POST["captcha_hash"]) || !isset($_POST["captcha"]))
            return;
        $hash = html_entity_decode($_POST["captcha_hash"], ENT_QUOTES, "UTF-8");
        $capt = $_POST["captcha"];
        //error_log("recv capt: $capt");
        //error_log("recv hash: $hash");
        if (strlen($capt) != $captcha_length) {
            //error_log("captcha hash doesn't match");
            return;
        }
        $h = md5($captcha_seed . $capt);
        //error_log("gen  hash: $h");
        if ($h != $hash) {
            //error_log("captcha hash doesn't match");
            return;
        }

        $q = "insert into comments (ip, name, email, website, title, text, article_id) values ('$ip', '$name', '$email', '$website', '$title', '$text', '$aid')";
        $r = query($q);
        pg_free_result($r);

        if ($email_notification["enabled"]) {
            //error_log("sending a notification about a comment to " . $email_notification["email"]);
            $subj = $name . " has commented on '" . get_article_title($aid) . "'";
            mail($email_notification["email"], $subj, $text);
        }
    }
    function delete_comment() {
        if (!isset($_GET["cid"]))
            return;
        $cid = $_GET["cid"];
        $q = "delete from comments where id = $cid";
        $r = query($q);
        pg_free_result($r);
    }
    function ban_ip_from_commenting() {
        if (!isset($_GET["cid"]))
            return;
        $cid = $_GET["cid"];
        $q = "select ip from comments where id = $cid";
        if (!($r = query($q)))
            return;
        if (!($row = pg_fetch_array($r, null, PGSQL_ASSOC))) {
            pg_free_result($r);
            return;
        }
        $ip = $row["ip"];
        pg_free_result($r);

        $q = "insert into banned_ips (ip) values ('$ip')";
        $r = query($q);
        pg_free_result($r);
    }
    function get_val(&$array, $name, $default = false) {
        if (isset($array[$name]))
            return $array[$name];
        return $default;
    }
    function is_banned_ip($ip) {
        $retval = false;
        $q = "select ip from banned_ips where ip = '$ip'";
        if (!($r = query($q)))
            return $retval;
        if ($row = pg_fetch_array($r, null, PGSQL_ASSOC)) {
            //error_log("this IP is banned");
            $retval = true;
        }
        pg_free_result($r);
        return $retval;
    } 
    function get_all_tags($page = -1) {
        $a = array();
        if ($page >= 0) {
            $pid = page2pid($page);
            $q = "select distinct tag from tags where article_id in (select id from articles where page_id = $pid) order by tag asc";
        } else {
            $q = "select distinct tag from tags order by tag asc";
        }
        if (!($r = query($q)))
            return $a;
        $i = 0;
        while ($row = pg_fetch_array($r, null, PGSQL_ASSOC)) {
            $a[$i] = $row["tag"];
            ++$i;
        }
        pg_free_result($r);
        return $a;
    }
    function get_tag_usage_counts($tags, $page = -1) {
        $a = array();
        if ($page != -1)
            $pid = page2pid($page);
        for ($i = 0; $i < sizeof($tags); ++$i) {
            if ($page != -1)
                $q = "select count(article_id) from tags where tag = '$tags[$i]' and article_id in (select id from articles where page_id = $pid)";
            else
                $q = "select count(article_id) from tags where tag = '$tags[$i]'";
            if (!($r = query($q))) {
                $a[$i] = 1;
                continue;
            }
            $row = pg_fetch_array($r, null, PGSQL_ASSOC);
            $a[$i] = $row["count"];
            pg_free_result($r);
        }
        return $a; 
    } 
    function display_tag_page() {
        echo "<div class=\"site_tags\">\n";
        display_tag_cloud();
        echo "</div>";
    }
    function display_tag_cloud($page = -1) {
        $tags = get_all_tags($page);
        if (sizeof($tags) < 1)
            return;
        $ntags = sizeof($tags);
        $tag_usage_counts = get_tag_usage_counts($tags, $page);
        $m = max($tag_usage_counts);
        $styles = array( // from smaller to larger
            //"font-size: xx-small",
            "font-size: x-small",
            "font-size: small",
            "font-size: medium",
            "font-size: large",
            "font-size: x-large",
            //"font-size: xx-large"
        );
        $nstyles = sizeof($styles);

        echo "<div class=\"tag_cloud\">\n";
        $page_str = ($page == -1 ? "" : "&page=$page");
        for ($i = 0; $i < $ntags; ++$i) {
            $tag = $tags[$i];
            if ($m == 1) { // each tag was used only once, display, them in medium font
                $index = intval(round($nstyles / 2));
            } else {
                $index = intval(round(($tag_usage_counts[$i] / $m) * ($nstyles - 1)));
            }
            $style = $styles[$index];
            if ($i > 0)
                echo ", ";
            echo "<a class=\"tag_cloud_link\" href=\"main.php?tag=$tag$page_str\"><span style=\"$style\">$tag</a>";
        }
        echo "</div>";
    } 
    function get_next_page_prio() {
        $q = "select max(priority) from pages";
        $r = query($q);
        if ($r) {
            $row = pg_fetch_array($r, null, PGSQL_ASSOC);
            pg_free_result($r);
            $max = $row["max"];
            return strval(intval($max) + 10);
        } else {
            return "10";
        }
    }
    function add_page() {
        if (!isset($_POST["title"]))
            return;

        $title = $_POST["title"];
        $descr = isset($_POST["descr"]) ? $_POST["descr"] : "";
        $prio = isset($_POST["prio"]) ? $_POST["prio"] : "";
        if ($prio == "")
            $prio = get_next_page_prio(); 

        $q = "insert into pages (title, descr, priority) values ('$title', '$descr', $prio)";
        $r = query($q);
        pg_free_result($r);
    }
    function mod_pages() {
        echo "<pre>";
        print_r($_POST["id"]);
        print_r($_POST["title"]);
        print_r($_POST["descr"]);
        print_r($_POST["priority"]);
        echo "</pre>\n";

        $id = $_POST["id"];
        $title = $_POST["title"];
        $descr = $_POST["descr"];
        $priority = $_POST["priority"];

        for ($i = 0; $i < sizeof($id); ++$i) {
            $pid = $id[$i];
            $t = htmlspecialchars_decode($title[$i]);
            $d = htmlspecialchars_decode($descr[$i]);
            $p = htmlspecialchars_decode($priority[$i]);

            $q = "update pages set title = '$t', descr = '$d', priority = $p where id = $pid";
            $r = query($q);
            free_result($r);
        } 
    }
    function display_mod_pages_page() {
        $q = "select id, title, descr, priority from pages";
        $r = query($q);
        echo "<form action=\"main.php\" method=\"post\">\n";
        while ($row = fetch_assoc_array($r)) {
            $id = $row["id"];
            $title = htmlspecialchars($row["title"]);
            $descr = htmlspecialchars($row["descr"]);
            $prio = htmlspecialchars($row["priority"]);

            echo "<input type=\"hidden\" name=\"id[]\" value=\"$id\">\n";
            echo "Title: <input type=\"text\" name=\"title[]\" value=\"$title\"><br>\n";
            echo "Descr: <input type=\"text\" name=\"descr[]\" value=\"$descr\"><br>\n";
            echo "Prio: <input type=\"text\" name=\"priority[]\" value=\"$prio\"><br><br>\n";
        }
        echo "<input type=\"hidden\" name=\"action\" value=\"mod_pages\">\n";
        echo "<input type=\"submit\" value=\"Submit\">\n";
        echo "</form>";
    }
    function delete_page() {
        global $page;

        $q = "delete from tags where article_id in (select id from articles where page_id = $page)";
        free_result(query($q));
        $q = "delete from articles where page_id = $page";
        free_result(query($q));
        $q = "delete from pages where id = $page";
        free_result(query($q));
    }



    $page = isset($_GET["page"]) ? intval($_GET["page"]) : -1;
    $pid = page2pid($page);
    $aid = isset($_GET["aid"]) ? intval($_GET["aid"]) : -1;
    $skip = isset($_GET["skip"]) ? intval($_GET["skip"]) : 0;

    if ($aid < -1)
        $aid = -1;
    if ($skip < 0)
        $skip = 0;
    
    //error_log("pid = $pid");
?>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<meta name="google-site-verification" content="0A_pJxLHCtyBKTGezlKoskVdE9uOGGUc0uJmRRrRghk" />
<title>S010's homepage</title>
<script type="text/javascript">
var gaJsHost = (("https:" == document.location.protocol) ? "https://ssl." : "http://www.");
document.write(unescape("%3Cscript src='" + gaJsHost + "google-analytics.com/ga.js' type='text/javascript'%3E%3C/script%3E"));
</script>
<script type="text/javascript">
try {
var pageTracker = _gat._getTracker("UA-11667539-1");
pageTracker._trackPageview();
} catch(err) {}</script>
<?php
    require("javascript.php");
?>
<style type="text/css">
<?php
    require("style.php");
?>
</style>
</head>
<body>
<div>
    <h1 class="site_title">S010's homepage</h1>
    <?php
        display_login_status();
        display_menu();
        if ($priv_level == ADMIN) {
            echo "<a href=\"main.php?action=mod_pages\">Modify pages</a><br>";
            echo "<form action=\"main.php\" method=\"post\">Title: <input type=\"text\" name=\"title\"> Descr: <input type=\"text\" name=\"descr\"> Prio: <input type=\"text\" name=\"prio\"> <input type=\"hidden\" name=\"action\" value=\"add_page\"><input type=\"submit\" value=\"Add page\"></form>";
        } 
        echo "<hr>\n";

        if (isset($_GET["tag"]))
            $tag = htmlspecialchars(html_entity_decode($_GET["tag"], ENT_QUOTES, "UTF-8"), ENT_QUOTES);
        else
            $tag = false;
            
        if ($get_action != "") {
            //error_log("get_action not empty");
            if ($get_action == "login") 
                require("login.php");

            if ($priv_level == ADMIN) {
                if ($get_action == "add_article")
                    display_add_article();
                else if ($get_action == "edit_article")
                    display_edit_article();
                else if ($get_action == "del_article")
                    delete_article($aid);
                else if ($get_action == "del_comment")
                    delete_comment();
                else if ($get_action == "ban_ip")
                    ban_ip_from_commenting();
                else if ($get_action == "mod_pages")
                    display_mod_pages_page();
                else if ($get_action == "delete_page")
                    delete_page();
            }

        } else if ($post_action != "") {
            //error_log("post_action not empty");
            if ($priv_level == ADMIN) {
                if ($post_action == "add_article")
                    add_article();
                else if ($post_action == "edit_article")
                    edit_article();
                else if ($post_action == "add_page")
                    add_page();
                else if ($post_action == "mod_pages")
                    mod_pages();
            }
            if ($post_action == "post_comment")
                post_comment();
        } 

        if ($get_action == "tags")
            display_tag_page();
        else if ($aid == -1) {
            if ($priv_level == ADMIN)
                display_page_admin_ctrls();
            display_page($page, $tag, $skip);
        } else {
            display_article($aid);
        }

        require("disconnect.php");
    ?>
</div>
</body>
</html>
