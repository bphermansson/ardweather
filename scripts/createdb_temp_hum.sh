rrdtool create temp_hum_db.rrd \
--step 60 \
DS:t1:GAUGE:120:-40:50 \
DS:h1:GAUGE:120:0:100 \
DS:b1:GAUGE:120:0:2 \
DS:t2:GAUGE:120:-40:50 \
DS:h2:GAUGE:120:0:100 \
DS:b2:GAUGE:120:0:2 \
RRA:MAX:0.5:1:1500 

# 120 is heartbeat timeout.  If data is not entered in at least 120 seconds then zeros are put into this DS record
#
#
