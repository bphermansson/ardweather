rrdtool create wash_db.rrd \
DS:t1:GAUGE:600:-40:50 \
DS:h1:GAUGE:600:0:100 \
DS:b1:GAUGE:600:0:2 \
RRA:AVERAGE:0.5:1:1200 \
