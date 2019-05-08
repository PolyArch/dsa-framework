def get_option_or_default(options, param, default):
    try:
        res = options
        for i in param:
            res = res[i]
        return res
    except:
        return default

def find_line(s, raw):
    return [i for i in raw if s in i]
def extract_essence(s, raw):
    return [i[len(s):].strip() for i in find_line(s, raw)]

def get_number_of(s, raw):
    try:
        return float(extract_essence(s, raw)[0])
    except:
        return None

def get_and_set_mus(parsed, attr, s, raw):
    val = get_number_of(s, raw)
    parsed[attr] = val
    if val is not None:
        parsed['mus'] = val

def parse_log(file_name, mode = 'single-core', opt = 'default'):
    print file_name
    result = {}
    raw = open(file_name, 'r').readlines()

    get_and_set_mus(result, 'ticks',  'ticks:', raw)
    if mode == 'single-core':
        get_and_set_mus(result, 'asic',   'ASIC Ideal:', raw)
    else:
        get_and_set_mus(result, 'asic',   'ASIC Latency:', raw)
    get_and_set_mus(result, 'cycles', 'Cycles:', raw)

    #print result
    assert result['mus'] is not None
    if 'mkl' not in file_name:
        result['mus'] *= 0.0008

    brkd = []
    for i in extract_essence('Cycle Breakdown:', raw):
        cur = {}
        for j in i.split():
            key, val = j.split(':')
            val = float(val)
            cur[key] = val
        brkd.append(cur)

    if not brkd:
        return result
    #print 'opt', opt
    if opt == 'merge-compute':
        delta = 0.
        #print brkd
        for elem in brkd[1:]:
            delta += elem['ISSUED']
            delta += elem['ISSUED_MULTI']
            delta += elem['CONFIG']
            brkd[0]['ISSUED'] += elem['ISSUED']
            brkd[0]['ISSUED_MULTI'] += elem['ISSUED_MULTI']
            brkd[0]['CONFIG'] += elem['CONFIG']
        brkd = brkd[0]
        total = sum(brkd[elem] for elem in brkd.iterkeys())
        for elem in brkd.iterkeys():
            brkd[elem] /= total
        #print 'origin:', result['mus']
        result['mus'] += delta * result['mus']
        #print 'origin:', result['mus']
    else:
        brkd0 = brkd[0]
        for elem in brkd[1:]:
            for key in elem.iterkeys():
                brkd0[key] += elem[key]
        length = len(brkd)
        for key in brkd0.iterkeys():
            brkd0[key] /= length
        brkd = brkd0

    result['breakdowns'] = brkd


    return result

def get_path_by_option(options, case, benchmark, impl, default_arch, defalut_size):
    should_apply = get_option_or_default(options, [benchmark, impl, 'when-case'], -1)
    should_apply = should_apply == -1 or should_apply == case
    if should_apply:
        log_name   = get_option_or_default(options, [benchmark, impl, 'arch'], default_arch)
        folder     = get_option_or_default(options, [benchmark, impl, 'folder'], benchmark)
        log_folder = get_option_or_default(options, [benchmark, impl, 'size-%d' % case], defalut_size)
        opt        = get_option_or_default(options, [benchmark, 'opt'], 'default')
    else:
        log_name   = default_arch
        folder     = benchmark
        log_folder = str(defalut_size)
        opt        = 'default'
    log_folder = 'log_%s' % log_folder
    return '../%s/%s/%s.log' % (folder, log_folder, log_name)

