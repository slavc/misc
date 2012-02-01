#!/bin/sh

max_fail_count=3

usage() {
}

print_links() {
    awk 'function extract_link(str,    s, e) {
            s = index($0, ">");
            e = index($0, "</");
            return substr($0, s + 1, e - s - 1);
        } 
        BEGIN {
            image_link = "";
            thumb_link = "";
        }
        /image_link/ {
            image_link = extract_link($0);
        }
        /thumb_link/ {
            thumb_link = extract_link($0);
        }
        END {
            print thumb_link "\t" image_link
        }' < "$1"
}

if [ $# -lt 1 ]; then
    usage
    exit 1
fi

if [ "$HOME" ]; then
    tmpfile="${HOME}/.up_img__tmp"
else
    tmpfile="/tmp/.up_img__tmp"
fi
    
while [ "$1" ]; do
    if ! [ -r "$1" ]; then
        shift
        continue
    fi

    fname=$(basename "$1")

    echo "# ${fname}"

    fail_count=0
    while [ $fail_count -lt $max_fail_count ]; do
        if curl -s -H Expect: -F "fileupload=@${1}" -F xml=yes http://www.imageshack.us/index.php > "$tmpfile" && egrep '<image_link>http.*' "$tmpfile" > /dev/null 2>&1; then
            break
        fi
        fail_count=$(($fail_count+1))
    done

    if [ $fail_count -ge $max_fail_count ]; then
        echo "# FAILED"
    else
        print_links "$tmpfile"
    fi

    shift
done

rm "$tmpfile"
