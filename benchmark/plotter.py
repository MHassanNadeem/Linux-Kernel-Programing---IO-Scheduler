'''
This script must be run in super user or sudo
'''

import os
import csv

CFQ_DATA_FILE = 'cfq.csv'
COOP_DATA_FILE = 'coop.csv'
CFQ_PLOT_FILE = 'cfq.png'
COOP_PLOT_FILE = 'coop.png'
BOTH_PLOT_FILE = 'cfq_coop.png'

def run_test(outfile):
    os.system(r'blktrace -d /dev/sda -a issue -o - | blkparse -q -f "%%t,%%S\n" -i - > %s &'%outfile)
    os.system("fio iometer-file-access-server.fio > /dev/null")
    os.system('pkill blktrace')

def set_sched(sched_name):
    if os.system(r'echo %s > /sys/block/sda/queue/scheduler'%sched_name) != 0:
        print('ERROR could not change the scheduler')

def read_csv(filename):
    with open(filename, 'rb') as csvfile:
        csvreader = csv.reader(csvfile, delimiter=',', quotechar='|')
        data = list(csvreader)
    return data

def write_csv(filename, data):
    with open(filename, 'wb') as csvfile:
        csvwriter = csv.writer(csvfile, delimiter=',', quotechar='|', quoting=csv.QUOTE_MINIMAL)
        for row in data:
            csvwriter.writerow(row)
    
def process_blktrace(infile, outfile):
    data = read_csv(infile)
    data.sort(key=lambda tup: tup[0])
    write_csv(outfile, data)
    return
    
def single_plot(label, data, outfile):
    cmd = "gnuplot -e \"data='%s'; label='%s'; out_file='%s'\" single_plot.gp"%(data,label,outfile)
    os.system(cmd)
    return
    
def dual_plot(label1, data1, label2, data2, outfile):
    cmd = "gnuplot -e \"data1='%s'; data2='%s'; label1='%s'; label2='%s'; out_file='%s'\" dual_plot.gp"%(data1,data2,label1,label2,outfile)
    os.system(cmd)
    return

print 'Collecting Data for CFQ'
set_sched('cfq')
run_test(CFQ_DATA_FILE)
process_blktrace(CFQ_DATA_FILE, '_'+CFQ_DATA_FILE)
single_plot('CFQ', '_'+CFQ_DATA_FILE, CFQ_PLOT_FILE)

print 'Collecting Data for COOP'
set_sched('coop')
run_test(COOP_DATA_FILE)
process_blktrace(COOP_DATA_FILE, '_'+COOP_DATA_FILE)
single_plot('COOP', '_'+COOP_DATA_FILE, COOP_PLOT_FILE)

dual_plot('CFQ', '_'+CFQ_DATA_FILE, 'COOP', '_'+COOP_DATA_FILE, BOTH_PLOT_FILE);