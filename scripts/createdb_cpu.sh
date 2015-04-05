rrdtool create cpu_db.rrd \
--step 60 \
DS:usage:GAUGE:120:0:100 \
RRA:MAX:0.5:1:1500 \
