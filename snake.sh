#!/bin/bash

# A simple clone of the Snake game written in Bourne shell.
#
# Works only in bash because of the usage of the non-standard "-n" option
# in "read" built-in command call.

keyb_input_file="${HOME}/.input_file_$$"

function handle_sigint {
    rm -f "$keyb_input_file"
    clear
    reset
    exit 0
}

trap handle_sigint SIGINT

function die {
    exit_status="$1"
    error_msg="$2"

    clear
    reset

    if [ "$error_msg" ]; then
        echo "$error_msg" 1>&2
    fi

    exit $exit_status
}

function reset_game {
    cols=`tput cols`
    rows=`tput lines`

    # head -> tail
    snake="hhhhhhhhhhhhhhhhhhhh"
    snake_len=${#snake}
    snake_x=`expr $cols / 2`
    snake_y=`expr $rows / 2`
    snake_has_crushed_into_wall=0
    snake_has_crushed_into_self=0
    snake_first_draw=1

    speed=5
    max_speed=20
    delay=`dc -e "2k 1 ${speed} / p"`
    score=0

}

function press_any_key {
    i=0
    ban=`echo -e ' #####                                    ###\n#     #  #    #    ##    #    #  ######   ###\n#        ##   #   #  #   #   #   #        ###\n #####   # #  #  #    #  ####    #####     #\n      #  #  # #  ######  #  #    #\n#     #  #   ##  #    #  #   #   #        ###\n #####   #    #  #    #  #    #  ######   ###\n\n'`
    banw=$(echo "$ban" | head -1 | wc -c)
    banh=$(echo "$ban" | wc -l)
    banx=$((${cols}/2-$banw/2))
    bany=$((${rows}/2-$banh/2))
    press="Press any key to start..."
    pressw=$(echo "$press" | wc -c)
    pressx=$((${cols}/2-$pressw/2))
    pressy=$(($bany+$banh+1))
    flip=0
    while [ -a "$keyb_input_file" ]; do
        if [ -s "$keyb_input_file" ]; then
            break
        fi

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

function put_food_at_random_coords {
    food_x=`expr $RANDOM \% $cols`
    food_y=`expr $RANDOM \% $rows`
    goto_xy $food_x $food_y
    echo -n '@'
}

function move_snake {
    if [ $snake_has_crushed_into_wall -ne 0 ] || [ $snake_has_crushed_into_self -ne 0 ]; then
        return
    fi

    if [ "$1" ]; then
        new_dir=$1
    else
        new_dir=${snake:0:1}
    fi

    new_snake=${snake:0:1}
    i=0
    n=$(($snake_len-1))
    while [ $i -lt $n ]; do
        new_snake=${new_snake}${snake:${i}:1}
        i=$(($i+1))
    done
    snake=$new_snake

    snake=${new_dir}${snake:1}
    case $new_dir in
    h)
        snake_x=$((${snake_x}-1))
        ;;
    j)
        snake_y=$((${snake_y}+1))
        ;;
    k)
        snake_y=$((${snake_y}-1))
        ;;
    l)
        snake_x=$((${snake_x}+1))
        ;;
    esac

    # check if it has collided with a wall
    if [ $snake_x -lt 0 ] || [ $snake_x -ge $cols ]; then
        snake_has_crushed_into_wall=1
        return
    fi
    if [ $snake_y -lt 0 ] || [ $snake_y -ge $rows ]; then
        snake_has_crushed_into_wall=1
        return
    fi

    if [ $snake_x -eq $food_x ] && [ $snake_y -eq $food_y ]; then
        put_food_at_random_coords
        score=$((${score}+1))
        i=$((${#snake}-1))
        snake=${snake}${snake:${i}:1}
        snake_len=${#snake}
        if [ $speed -lt $max_speed ]; then
            speed=$((${speed}+1))
            delay=`dc -e "2k 1 ${speed} / p"`
        fi
    fi
}

function goto_xy {
    echo -ne "\033[${2};${1}H"
}

function draw_snake {
    cur_node=""
    prev_node=""
    cur_x=$snake_x
    cur_y=$snake_y

    if [ $snake_first_draw -eq 1 ]; then
        i=0
        while [ $i -lt $snake_len ]; do
            cur_node=${snake:${i}:1}
            goto_xy $cur_x $cur_y
            echo -n '#'
            if [ "$prev_node" ]; then
                case $prev_node in
                h)
                    cur_x=$((${cur_x}+1))
                    ;;
                j)
                    cur_y=$((${cur_y}-1))
                    ;;
                k)
                    cur_y=$((${cur_y}+1))
                    ;;
                l)
                    cur_x=$((${cur_x}-1))
                    ;;
                esac

                if [ $snake_x -eq $cur_x ] && [ $snake_y -eq $cur_y ]; then
                    snake_has_crushed_into_self=1
                fi
            fi
            prev_node=$cur_node
            i=$((${i}+1))
        done
    else
        i=0
        while [ $i -lt $snake_len ]; do
            cur_node=${snake:${i}:1}
            if [ "$prev_node" ]; then
                case $prev_node in
                h)
                    cur_x=$((${cur_x}+1))
                    ;;
                j)
                    cur_y=$((${cur_y}-1))
                    ;;
                k)
                    cur_y=$((${cur_y}+1))
                    ;;
                l)
                    cur_x=$((${cur_x}-1))
                    ;;
                esac

                if [ $snake_x -eq $cur_x ] && [ $snake_y -eq $cur_y ]; then
                    snake_has_crushed_into_self=1
                fi
            fi
            prev_node=$cur_node
            i=$((${i}+1))
        done

        goto_xy $cur_x $cur_y
        echo -n ' '

        goto_xy $snake_x $snake_y
        echo -n '#'
    fi

    goto_xy $food_x $food_y

    snake_first_draw=0
}

touch "$keyb_input_file" || die 1 "Failed to create input file"
reset_game

parent_pid=$$

# main loop
{
    press_any_key
    clear

    put_food_at_random_coords
    while [ -a "$keyb_input_file" ]; do
        input=`cat "$keyb_input_file"`
        > "$keyb_input_file"
        
        # snake is allowed to turn only 90 degrees at a time
        new_dir=""
        case ${snake:0:1}${input} in
        (hj|hk|lj|lk|jh|jl|kh|kl)
            new_dir=$input
            ;;
        esac

        if [ $snake_has_crushed_into_wall -ne 0 ] || [ $snake_has_crushed_into_self -ne 0 ]; then
            msg="GAME OVER"
            x=$((${cols}/2-${#msg}/2))
            y=$((${rows}/2))
            goto_xy $x $y
            echo -n "$msg"
            while [ -a "$keyb_input_file" ]; do
                if [ -s "$keyb_input_file" ]; then
                    rm -f "$keyb_input_file"
                    clear
                    reset
		    kill $parent_pid
                    exit
                fi
                sleep 0.3
            done
        else
            move_snake $new_dir
            draw_snake
        fi

        sleep $delay
    done

    clear
    reset
} &

while [ -a "$keyb_input_file" ]; do
    read -s -n 1 keyb_input
    echo "$keyb_input" > "$keyb_input_file"
done
