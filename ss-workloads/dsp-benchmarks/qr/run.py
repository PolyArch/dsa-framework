#!/usr/bin/env python3
import imp, os

run = imp.load_source('run', '../tools/run.py')

run.run([12, 32], 'N=%d ', ['origin', 'new', 'latency'], 'qr.res')

SS = os.getenv('SS')
run.run([12, 32], 'SBCONFIG=%s/ss-scheduler/configs/revel-1x2.sbmodel N=%s ' % (SS, '%d'), ['new'], 'qr.res')
run.run([12, 32], 'SBCONFIG=%s/ss-scheduler/configs/revel-2x2.sbmodel N=%s ' % (SS, '%d'), ['new'], 'qr.res')
run.run([12, 32], 'SBCONFIG=%s/ss-scheduler/configs/revel-3x2.sbmodel N=%s ' % (SS, '%d'), ['new'], 'qr.res')
run.run([12, 32], 'SBCONFIG=%s/ss-scheduler/configs/revel-3x3.sbmodel N=%s ' % (SS, '%d'), ['new'], 'qr.res')
