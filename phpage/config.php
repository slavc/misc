<?php
    $connect_string = "host=localhost dbname=phpage user=phpage password=phpage";
    $articles_per_page = 5;
    $captcha_length = 5;
    $captcha_seed = "(**ssjdqw*(ewe0988wq8723ndsdj";

    $lj_cross_post = Array(
        "enabled" => false,
        "username" => "username",
        "password" => "password",
        "comment" => "<p style=\"font-size: small\">Crossposted from <a href=\"myurl/\">myurl</a>.</p>",
        "prog_fmt" => "./ljpost.py -u \"%s\" -p \"%s\" -s \"%s\" -k Gentleman \"%s\"" 
    ); 

    $crossposters = Array(
        $lj_cross_post
    );

    $email_notification = Array(
        "enabled" => true,
        "email" => "email@address.com"
    );
