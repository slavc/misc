#!/bin/sh

charge_now=`cat '/sys/bus/acpi/drivers/battery/PNP0C0A:00/power_supply/BAT0/charge_now'`
charge_full=`cat '/sys/bus/acpi/drivers/battery/PNP0C0A:00/power_supply/BAT0/charge_full'`
status=`cat '/sys/bus/acpi/drivers/battery/PNP0C0A:00/power_supply/BAT0/status'`

charge=`bc << EOF
scale=2
x=($charge_now/$charge_full)*100
x
EOF`

if [ "$status" == "Unknown" ]; then
    status="Fully charged"
fi

echo ${charge%%.*}% $status

