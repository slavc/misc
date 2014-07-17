<?php
    function sql_error($q) {
        die("Query \"" . $q . "\" failed: " . pg_last_error());
    }
    function query($q) {
        $r = pg_query($q) or sql_error($q);
        return $r;
    }
    function fetch_assoc_array($r) {
        return pg_fetch_array($r, null, PGSQL_ASSOC);
    }
    function free_result($r) {
        pg_free_result($r);
    }
