<script type="text/javascript">
function set_page_descr(text) {
    var div;

    if (!(div = document.getElementById("div_page_descr")))
        return;
    div.textContent = text;
}
function check_captcha() {
    var code, hash;

    code = document.getElementById("captcha_code");
    hash = document.getElementById("plain_captcha_hash");
    if (!code || !hash) {
        return false;
    }
    if (MD5(code.value) != hash.value) {
        alert("The code you entered is incorrect!");
        return false;
    } else {
        return true;
    }
}
</script>
<script type="text/javascript" src="md5.js"></script>

