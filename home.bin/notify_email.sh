#!/bin/sh

summary=Email
n_msgs=${1:-1}
body=$(printf 'You have %d new message' $n_msgs)
if [ $n_msgs -gt 1 ]; then
    body=${body}s
fi

exec /usr/bin/notify-send -i /usr/share/icons/Faenza/status/48/notification-message-email.png Email "$body"
