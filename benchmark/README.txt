== Modify kernel code ==
* Please refer to "coop-iosched" in the source code
> $insmod coop-iosched.ko
> echo coop > /sys/block/sda/queue/scheduler


== Run benchmark ==
fio tools/iometer-file-access-server.fio


== Block trace ==
run_blktrace.sh
