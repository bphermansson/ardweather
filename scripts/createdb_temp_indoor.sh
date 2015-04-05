rrdtool create temp_indoor_db.rrd \
DS:t1:GAUGE:600:-40:50 \
DS:b1:GAUGE:600:0:2 \
RRA:AVERAGE:0.5:1:1200 \


# 120 is heartbeat timeout.  If data is not entered in at least 120 seconds then zeros are put into this DS record
#
#
