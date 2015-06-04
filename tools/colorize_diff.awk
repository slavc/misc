#!/usr/bin/awk -f

function append_attr(s, n) {
    if (n == -1)
        return s;
    if (length(s) > 0)
        s = s ";";
    return sprintf("%s%d", s, n);
}

function set_attrs(bold, fg, bg) {
    if (fg != -1)
        fg += 30;
    if (bg != -1)
        bg += 40;
    s = "";
    s = append_attr(s, bold);
    s = append_attr(s, fg);
    s = append_attr(s, bg);
    printf("\033[%sm", s);
    need_reset = 1;
}

function reset_attrs() {
    if (need_reset) {
        printf("\033[0m");
        need_reset = 0;
    }
}

BEGIN {
    NA = -1;

    BOLD = 1;

    BLACK = 0;
    RED = 1;
    GREEN = 2;
    YELLOW = 3;
    BLUE = 4;
    MAGENTA = 5;
    CYAN = 6;
    WHITE = 7;

    need_reset = 0;
}

# Name of changed file / name of new file.
/^\+\+\+/ {
    set_attrs(BOLD, WHITE, BLUE);
}

# Lines which have been added.
/^\+[^+]/ {
    set_attrs(BOLD, WHITE, GREEN);
}

# Lines which have been removed.
/^-[^-]/ {
    set_attrs(BOLD, WHITE, RED);
}

{
    print;
    reset_attrs();
}
