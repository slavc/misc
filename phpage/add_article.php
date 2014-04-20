<?php
function display_add_article() {
	global $page;

	$form_action = "main.php?page=$page";
	add_edit_article($form_action, "add_article", "", "", "", "");
}

function display_edit_article() {
	global $page, $aid;

	$q = "select id, title, text from articles where id = $aid";
	if (!($r = query($q)))
		return;
	$row = pg_fetch_array($r, null, PGSQL_ASSOC);
    $aid = $row["id"];
	$title = $row["title"];
	$text = $row["text"];
	pg_free_result($r);

	$form_action = "main.php?page=$page";
    // XXX tags
    $tags = get_article_tags($aid);
    $tag_str = implode(", ", $tags);
	add_edit_article($form_action, "edit_article", $title, $tag_str, $text, $aid);
}

function add_edit_article($form_action, $action, $title, $tags, $text, $aid) {
echo "<div class=\"add_article\">\n
<form class=\"$action\" action=\"$form_action\" method=\"post\">\n
<table border=\"0\" cellspacing=\"5\">\n
<tr valign=\"top\"><td>Title:</td><td><input style=\"width: 100%\" type=\"text\" name=\"title\" value=\"$title\"></td></tr>\n
<tr valign=\"top\"><td>tags:</td><td><input style=\"width: 100%\" type=\"text\" name=\"tags\" value=\"$tags\"></td></tr>\n
<tr valign=\"top\"><td></td><td><input type=\"checkbox\" name=\"do_crosspost\" checked=\"1\" value=\"1\"> Do crossposts</td></tr>\n
<tr valign=\"top\"><td>Text:</td><td><textarea name=\"text\" cols=\"80\" rows=\"40\">$text</textarea></td></tr>\n
<tr valign=\"top\"><td colspan=\"2\"><div align=\"right\"><input type=\"submit\" value=\"Submit\"></div>\n
<input type=\"hidden\" name=\"action\" value=\"$action\"><input type=\"hidden\" name=\"aid\" value=\"$aid\"></td></tr>\n
</table>\n
</form>\n
</div>";
}

