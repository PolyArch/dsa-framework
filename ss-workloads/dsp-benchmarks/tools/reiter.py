import struct, sys

def reiterpret(n):
    if n >= 0 and n <= 4:
        return '%d' % n
    pack = struct.pack('Q', n)
    return '(%.5f %.5f)' % struct.unpack('ff', pack)

if __name__ == '__main__':
    # If it is run by itself, parse the data
    last_output = False
    for line in file('raw', 'r').readlines():
        if 'inputs ' in line:
            if last_output:
                print
            print 'Input:',
            for j in line.split(':')[1].rstrip().rstrip(',').split(','):
                print reiterpret(int(j, 16)), 
            print
            last_output = False
        elif 'output:' in line:
            print 'Output:',
            print reiterpret(int(line[7:-10], 16)),
            if 'valid:0' in line:
                print 'invalid'
            else:
                print 'valid'
            last_output = True
        else:
            elems = line.split()
            if not elems:
                continue
            print elems[0],
            for j in elems[1:]:
                try:
                    print reiterpret(int(j, 16)),
                except:
                    print j,
            print
            last_output = False

