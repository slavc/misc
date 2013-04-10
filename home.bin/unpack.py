#!/usr/bin/python

# Recursively unpacks an archive, until there are no archives and
# compressed files left.

import os, sys, stat, getopt, subprocess, tarfile, string

progname = os.path.basename(sys.argv[0])
debug_enabled = False

def warn(msg):
    sys.stderr.write('%s: %s\n' % (progname, msg))

def debug(msg):
    if debug_enabled:
        sys.stdout.write('%s\n' % msg)

def dec_max_depth(i):
    if i > 0:
        return i - 1
    else:
        return i

def remove_extension(path):
    pos = path.rindex('.')
    if pos == -1:
        return path
    path = path[:pos]
    if path.endswith('.tar'):
        return remove_extension(path)
    else:
        return path

def analyze_tar(tar_path):
    tar = tarfile.open(tar_path, 'r')
    members = tar.getmembers()
    top_level_dir = ''
    n_top_level = 0
    for memb in members:
        if '/' not in memb.name:
            if memb.isdir():
                top_level_dir = memb.name
            n_top_level += 1
        if n_top_level > 1:
            break
    tar.close()
    return (top_level_dir, n_top_level)

def compute_resulting_dir(tar_path):
    top_level_dir, n_top_level = analyze_tar(tar_path)

    resulting_dir = ''
    if n_top_level > 1 or not top_level_dir:
        resulting_dir = remove_extension(tar_path)
    else:
        resulting_dir = '%s/%s' % (os.path.dirname(tar_path) or '.', top_level_dir)
    return resulting_dir

# Return tar -C option's argument,
# i.e. where the tar should be unpacked.
def compute_target_dir(tar_path):
    top_level_dir, n_top_level = analyze_tar(tar_path)

    target_dir = ''
    if n_top_level > 1 or not top_level_dir:
        target_dir = remove_extension(tar_path)
    else:
        target_dir = os.path.dirname(tar_path) or '.'
    return target_dir

def exists(path):
    try:
        st = os.stat(path)
    except:
        return False
    return True

def is_dir(path):
    try:
        st = os.stat(path)
    except:
        return False
    return stat.S_ISDIR(st.st_mode)

def untar(path):
    target_dir = compute_target_dir(path)
    if exists(target_dir):
        if not is_dir(target_dir):
            warn('cannot unpack %s: target dir %s already exists and is not a directory' % (path, target_dir))
            return False
    else:
        try:
            os.mkdir(target_dir)
        except:
            warn('cannot unpack %s: failed to create target dir %s' % (path, target_dir))
            return False
    argv = []
    argv.append('tar')
    argv.append('-C')
    argv.append(compute_target_dir(path))
    if path.endswith('gz'):
        argv.append('-zxf')
    else:
        argv.append('-xf')
    argv.append(path)

    debug(string.join(argv))
    rc = subprocess.call(argv)
    if rc != 0:
        return False
    os.remove(path)
    return True

def gunzip(path):
    argv = ['gunzip', path]
    debug(string.join(argv))

    rc = subprocess.call(['gunzip', path])
    return rc == 0

def unpack(path, max_depth):
    debug('unpack(%s, %s)' % (path, max_depth))
    if max_depth == 0:
        return False

    try:
        st = os.stat(path)
    except:
        warn("%s: stat() failed" % path)
        return False

    if stat.S_ISDIR(st.st_mode):
        do_unpack = True
        while do_unpack:
            do_unpack = False
            filenames = os.listdir(path)
            for filename in filenames:
                if unpack('%s/%s' % (path, filename), dec_max_depth(max_depth)):
                    do_unpack = True
        return False
    elif stat.S_ISREG(st.st_mode):
        if path.endswith('.tar.gz') or path.endswith('.tgz') or path.endswith('.tar'):
            resulting_dir = compute_resulting_dir(path)
            if untar(path):
                unpack(resulting_dir, dec_max_depth(max_depth))
        elif path.endswith('.gz'):
            return gunzip(path)
        else:
            return False
    else:
        warn('%s: is not a directory nor a file', path)
        return False

def tuples2dict(tuples):
    d = { }
    for k, v in tuples:
        d[k] = v
    return d

def usage():
    print 'usage: %s [-d] [-m] <archive1> <archive2> <archiveN>...' %  progname
    print '  -d -- turn on debugging'
    print '  -m <int> -- max depth'

def main(args):
    global debug_enabled

    max_depth = -1

    (opts, paths) = getopt.getopt(args, 'm:d')
    opts = tuples2dict(opts)
    if '-m' in opts:
        max_depth = int(opts['-m'])
    if '-d' in opts:
        debug_enabled = True
    debug('opts=%s' % opts)
    debug('paths=%s' % paths)

    for path in paths:
        unpack(path, max_depth)

if __name__ == '__main__':
    main(sys.argv[1:])
