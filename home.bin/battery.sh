#!/bin/sh

energy_now=`cat '/sys/bus/acpi/drivers/battery/PNP0C0A:00/power_supply/BAT0/energy_now'`
energy_full=`cat '/sys/bus/acpi/drivers/battery/PNP0C0A:00/power_supply/BAT0/energy_full'`
status=`cat '/sys/bus/acpi/drivers/battery/PNP0C0A:00/power_supply/BAT0/status'`

charge=`bc << EOF
scale=2
x=($energy_now/$energy_full)*100
x
EOF`

if [ "$status" == "Unknown" ]; then
    status="Fully charged"
fi

echo ${charge%%.*}% $status

