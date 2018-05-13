#!/bin/sh

err() {
	exitcode=$1
	shift
	echo "$0: error:" "$@" 1>&2
	exit $exitcode
}

if [ $# -eq 0 ]; then
	echo "usage: $0 <destination_dir>"
	exit 1
fi

destination=$1

mkdir -p "$destination"/{static,cache} || err $? failed to create directories
cp index.html "$destination"/static || err $? failed to copy index.html
cp main.go "$destination"/webscruffy.go || err $? failed to copy main.go
touch "$destination"/log
cat > "$destination"/start_webscruffy.sh << "EOF" || err $? failed to create start-up script
#!/bin/sh

err() {
	exitcode=$1
	shift
	echo "$0: error:" "$@" 1>&2
	exit $exitcode
}

if [ "$GOPATH" == "" ]; then
	err 1 GOPATH environment variable is not set
fi

workdir=$(dirname "$0")
if [ "$workdir" == "" ]; then
	workdir="."
fi
cd "$workdir" || err $? failed to change into work directory "$workdir"
go run webscruffy.go "$@" < /dev/null 1>&2 >> log &
if [ $? -ne 0 ]; then
	err $? failed to start webscruffy
fi
EOF

find "$destination" -type f -exec chmod 600 '{}' ';'
chmod u+x "$destination"/start_webscruffy.sh || err $? failed to chmod +x start script
