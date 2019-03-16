#!/bin/sh

set -e

wifi=wlan1
wan=usb0
bridge=br0

log() {
	/usr/bin/logger --id=$$ -p local0.info -t $0 "$@"
}

fatal() {
	log "$@"
	exit 1
}

start_ap() {
	log trying to bring up an access point...

	/usr/bin/systemctl stop hostapd || fatal failed to stop hostapd: return code $?
	/usr/sbin/ip addr flush dev $wifi || fatal failed to flush IP addresses of $wifi: return code $?
	/usr/sbin/ip link set down $wifi || fatal failed to put the link $wifi down: return code $?
	/usr/bin/systemctl start hostapd || fatal failed to start hostapd: return code $?
	/usr/sbin/ip link set down $bridge || true
	/usr/sbin/brctl delbr $bridge || true
	/usr/sbin/brctl addbr $bridge || fatal failed to add bridge $bridge: return code $?
	/usr/sbin/brctl addif $bridge $wifi || fatal failed to add wlan interface $wifi into bridge $bridge: return code $?
	/usr/sbin/brctl addif $bridge $wan || fatal failed to add wan interface $wan into bridge $bridge: return code $?
	/usr/sbin/ip link set up $wan || fatal failed to bring wan interface $wan up: return code $?
	/usr/sbin/ip link set up $bridge || fatal failed to bring the bridge interface $bridge up: return code $?

	log successfully brought up the access point
}

stop_ap() {
	log bringing access point down...

	/usr/bin/systemctl stop hostapd || fatal failed to stop hostapd: return code $?
	/usr/sbin/ip addr flush dev $wifi || fatal failed to flush IP addresses of $wifi: return code $?
	/usr/sbin/ip link set down $wifi || fatal failed to put the link $wifi down: return code $?
	/usr/sbin/ip link set down $bridge || true
	/usr/sbin/brctl delbr $bridge || true

	log successfully brought down the access point
}

case "$1" in
start)
	start_ap
	;;
stop)
	stop_ap
	;;
esac
