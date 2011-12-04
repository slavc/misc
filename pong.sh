#!/usr/local/bin/bash

# A simple clone of the PONG videogame written in Bourne shell.
#
# Works only in bash because of the usage of the non-standard "-n" option
# in "read" built-in command call.

delay=0.05

com_file="${HOME}/.pong_com" # communication between shell and subshell
lock_file="${HOME}/.pong_lock"

player_score=0
cpu_score=0

# wall
wx=1
wy=1
ww=$(tput cols)
wh=$(tput lines)

pad_h=5

pad1_x=6
pad1_y=0
pad2_x=$(($ww-6))
pad2_y=0

ball_x=0
ball_y=0
ball_vx=-1
ball_vy=1

place_pads() {
    pad1_y=$(($wh/2-$pad_h/2))
    pad2_y=$pad1_y
}

place_ball() {
    ball_x=$(($ww/2))
    ball_y=$(($wh/2))
}

move_ball() {
    x=$(($ball_x+$ball_vx))
    y=$(($ball_y+$ball_vy))

    if [ $x -le $wx ]; then
        cpu_score=$(($cpu_score+1))
        x=$wx
        ball_vx=1
    elif [ $x -ge $ww ]; then
        player_score=$(($player_score+1))
        x=$ww
        ball_vx=-1
    fi

    if [ $y -le $wy ]; then
        y=$wy
        ball_vy=1
    elif [ $y -ge $wh ]; then
        y=$wh
        ball_vy=-1
    fi

    if [ $x -eq $pad1_x ] && [ $y -ge $pad1_y ] && [ $y -le $(($pad1_y+$pad_h)) ]; then
        ball_vx=$((-$ball_vx))
        move_ball
        return
    elif [ $x -eq $pad2_x ] && [ $y -ge $pad2_y ] && [ $y -le $(($pad2_y+$pad_h)) ]; then
        ball_vx=$((-$ball_vx))
        move_ball
        return
    fi

    ball_x=$x
    ball_y=$y
}

draw_pad() {
    x=$1
    y=$2
    i=0
    c="$3"
    while [ $i -lt $pad_h ]; do
        put_char $x $(($y+$i)) "$c"
        i=$(($i+1))
    done
}

redraw() {
    draw_pad $pad1_x $pad1_y '#'
    draw_pad $pad2_x $pad2_y '#'

    put_char $ball_x $ball_y '@'
    goto_xy $ball_x $ball_y
}

set_color() {
    printf "\033[${1}m"
}

goto_xy() {
    printf "\033[${2};${1}H"
}

put_char() {
    printf "\033[${2};${1}H%s" "$3"
}

on_quit() {
    rm -f "$lock_file"
    reset
    clear
    exit 0
}

erase() {
    put_char $ball_x $ball_y ' '
    draw_pad $pad1_x $pad1_y ' '
    draw_pad $pad2_x $pad2_y ' '
}

show_scores() {
    scores=$(printf "%i : %i" $player_score $cpu_score)
    l=$(echo -n $scores | wc -c)
    x=$(($ww/2-$l/2))
    goto_xy $x 0
    echo -n "$scores"
}

move_pad() {
    y=$1
    dy=$2
    ny=$(($y+$dy))
    ly=$(($ny+$pad_h))

    if [ $ny -lt $wy ]; then
        ny=$wy
    elif [ $ly -gt $wh ]; then
        ny=$(($wh-$pad_h+1))
    fi

    echo $ny
}

ai() {
    cy=$(($pad2_y+$pad_h/2))
    if [ $ball_vx -lt 0 ]; then
        # Follow player
        if [ $RANDOM -gt 10000 ]; then
            return
        fi
        d=$(($pad2_y-$pad1_y))
        if [ $d -lt 0 ]; then
            pad2_y=$(move_pad $pad2_y 1)
        elif [ $d -gt 0 ]; then
            pad2_y=$(move_pad $pad2_y -1)
        fi
    else
        # Follow ball
        d=$(($cy-$ball_y))
        if [ $d -lt 0 ]; then
            pad2_y=$(move_pad $pad2_y 1)
        elif [ $d -gt 0 ]; then
            pad2_y=$(move_pad $pad2_y -1)
        fi
    fi
}

handle_input() {
    inp=$(head -1 "$com_file")
    if [ -z "$inp" ]; then
        return;
    fi
    case "$inp" in
    (xj)
        pad1_y=$(move_pad $pad1_y 1)
        ;;
    (xk)
        pad1_y=$(move_pad $pad1_y -1)
        ;;
    esac
    > "$com_file"
}

press_any_key() {
    i=0
    #ban="$(banner Pong!)"
    ban=$(echo -e "######                            ###\n#     #   ####   #    #   ####    ###\n#     #  #    #  ##   #  #    #   ###\n######   #    #  # #  #  #         #\n#        #    #  #  # #  #  ###\n#        #    #  #   ##  #    #   ###\n#         ####   #    #   ####    ###\n\n")
    banw=$(echo "$ban" | head -1 | wc -c)
    banh=$(echo "$ban" | wc -l)
    banx=$(($ww/2-$banw/2))
    bany=$(($wh/2-$banh/2))
    press="Press any key to start..."
    pressw=$(echo "$press" | wc -c)
    pressx=$(($ww/2-$pressw/2))
    pressy=$(($bany+$banh+1))
    flip=0
    while [ -z "$(head -1 $com_file)" ] && [ -f "$lock_file" ]; do
        clear

        i=0
        while [ $i -lt $banh ]; do
            goto_xy $banx $(($bany+$i))
            lineno=$(($i+1))
            line=$(echo "$ban" | awk "NR==${lineno} { print }")
            echo -n "$line"
            i=$(($i+1))
        done

        if [ $flip -eq 0 ]; then
            goto_xy $pressx $pressy
            echo -n "$press"
            flip=1
        else
            goto_xy $(($pressx+$pressw-1)) $pressy
            flip=0
        fi

        sleep 0.5
    done
}

trap on_quit TERM INT QUIT

stty -echo

> "$lock_file"
> "$com_file"

{
    press_any_key
    clear
    place_pads
    place_ball
    while [ -f "$lock_file" ]; do
        erase
        handle_input
        ai
        move_ball
        show_scores
        redraw
        sleep $delay
    done
    rm -f "$com_file"
} &

while [ -f "$lock_file" ]; do
    read -n 1 inp
    echo x$inp > "$com_file"
done
rm -f "$com_file"

