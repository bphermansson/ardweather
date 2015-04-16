#!/bin/bash
#
### set the paths
rrdtool="/usr/bin/rrdtool"
top="/usr/bin/top"
grep="/bin/grep"
tail="/usr/bin/tail"
awk="/usr/bin/awk"

# Check Esic sensor House 1, channel 3 (attic)
esicvalue1=`python /home/pi/ardweather141217/serpy-ardweather150313.py 1 3`
echo $esicvalue1

# Split answer in values, separated by ";"
OIFS="$IFS"
IFS=';'
read -a values1 <<< "${esicvalue1}"
IFS="$OIFS"
echo ${values1[0]}  # Temp
echo ${values1[1]}  # Humidity
echo ${values1[2]}  # Battery

# Clean battery value
b1=`echo ${values1[2]} | sed 's/ *$//g'`
b1=`echo ${b1:0:1}`

### change to the script directory
#cd /root/ardweather131010

### update the database
# Temp & Humidity
$rrdtool update /home/pi/ardweather141217/attic_db.rrd N:${values1[0]}:${values1[1]}:$b1

