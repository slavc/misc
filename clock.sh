#!/bin/sh

# A sort of a screensaver for a terminal: displays a clock which moves
# around the screen and bounces off of the edges.

fg=34
bg=47

x=0
y=0
xd=0
yd=0
w=0
h=0
cw=0
ch=0
delay=1
blank=""

function gotoxy
{
	printf "\033[${2};${1}H"
}

function putchar
{
	printf "\033[${1};${2}H%s" $3
}

function setfg
{
	printf "\033[${1}m"
}
alias setbg=setfg
alias setgr=setfg

function setattr
{
	printf "\033[${1};${2};${3}m"
}

function init
{
	cw=$(print_clock | awk '{ print $0; exit 0; }' | wc -c | awk '{ print $1 }')
	ch=1

	w=$(tput cols)
	h=$(tput lines)

	x=$(expr $RANDOM % \( $w - $cw \) )
	y=$(expr $RANDOM % \( $h - $ch \) )

	if [ $RANDOM -gt 16536 ]; then
		xd=1
	else
		xd=-1
	fi
	if [ $RANDOM -gt 16536 ]; then
		yd=1
	else
		yd=-1
	fi

	i=1
	while [ $i -lt $cw ]; do
		blank=${blank}"X"
		i=$(expr $i + 1)
	done

	clear
}

function hidecursor
{
	printf "\033?25l"
}

function showcursor
{
	printf "\033?25h"
}

function print_clock
{
	date | awk '{ printf("%s", $4); }'
}

function wrap_coords
{
	if [ $x -le 1 ]; then
		xd=1
	elif [ $x -gt $(expr $w - $cw) ]; then
		xd=-1
	fi

	if [ $y -le 1 ]; then
		yd=1
	elif [ $y -gt $(expr $h - $ch) ]; then
		yd=-1
	fi

	x=$(expr $x + $xd)
	y=$(expr $y + $yd)

	trap handle_int 2
}

function handle_int
{
	printf "\033[0m"
	showcursor
	clear
	exit 0
}

function erase_to_bol
{
	printf "\033[1K"
}

init
setgr 0
setfg $fg
setbg $bg
while true; do
	clear
#	setattr 0 $fg $bg
	putchar $y $x $(print_clock)
	sleep $delay
#	setattr 0 30 40
#	erase_to_bol
	wrap_coords
done

