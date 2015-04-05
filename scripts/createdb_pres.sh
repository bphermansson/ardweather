rrdtool create pres_db.rrd \
--step 60 \
DS:p1:GAUGE:600:950:1050 \
RRA:MAX:0.5:1:1200 \
