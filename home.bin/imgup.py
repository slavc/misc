#!/usr/local/bin/python2.5

# Upload images to www.imageshack.us

import urllib, httplib, mimetypes, sys


debugon   = True
host      = "imageshack.us"
port      = 80
actionurl = "/"
fieldname = "fileupload"
brstr     = "<br>"
links     = []
filenames = []
opts      = {}

def debug(s):
    if not debugon:
        return
    print "dbg: " + s

def post_multipart(host, port, selector, fields, files):
    """
    Post fields and files to an http host as multipart/form-data.
    fields is a sequence of (name, value) elements for regular form
fields.
    files is a sequence of (name, filename, value) elements for data to
be uploaded as files
    Return the server's response page.
    """
    debug("in post_multipart")
    debug("host = %s; port = %s; selector = %s\n"%(str(host), str(port), str(selector)))
    content_type, body = encode_multipart_formdata(fields, files)

    h = httplib.HTTP(host, port)

    h.putrequest('POST', selector)
    h.putheader('content-type', content_type)
    h.putheader('content-length', str(len(body)))
    h.endheaders()
    h.send(body)
    errcode, errmsg, headers = h.getreply()
    return h.file.read()
#    try:
#        h.send(body)
#        errcode, errmsg, headers = h.getreply()
#        return h.file.read()
#    except:
#        return None

def encode_multipart_formdata(fields, files):
    debug("in encode_multipart_formdata")
    """
    fields is a sequence of (name, value) elements for regular form
fields.
    files is a sequence of (name, filename, value) elements for data to
be uploaded as files
    Return (content_type, body) ready for httplib.HTTP instance
    """
    BOUNDARY = '---------------------------13049614110900'

    CRLF = '\r\n'
    L = []
    for (key, value) in fields:
        L.append('--' + BOUNDARY)
        L.append('Content-Disposition: form-data; name="%s"' % key)
        L.append('')
        L.append(value)
    for (key, filename, value) in files:
        L.append('--' + BOUNDARY)
        L.append('Content-Disposition: form-data; name="%s";\
filename="%s"' % (key, filename))
        L.append('Content-Type: %s' % get_content_type(filename))
        L.append('')
        L.append(value)
    L.append('--' + BOUNDARY + '--')
    L.append('')
    body = CRLF.join(L)
    content_type = 'multipart/form-data; boundary=%s' % BOUNDARY
    return content_type, body

def get_content_type(filename):
    return mimetypes.guess_type(filename)[0] or 'application/octet-stream'









def procargs():
    debug("in procargs")
    global filenames, opts
    
    opts["c"]   = False
    opts["s"]   = False
    opts["br"]  = 0
    opts["T"]   = 0
    opts["tw"]  = True
    opts["tf1"] = False
    opts["tf2"] = False
    opts["hw"]  = False
    opts["hf1"] = False
    opts["hf2"] = False
    
    twmet = False
    
    for arg in sys.argv[1:]:
        if arg.startswith("-"):
            opt = arg[1:]
            if opt == "h":
                usage()
            elif opt == "tw":
                twmet = True
            elif opt in ("tf1", "tf2", "hw", "hf1", "hf2"):
                opts["tw"] = False
                opts[opt] = True
            elif opt == "c":
                opts["c"] = True
            elif opt == "s":
                opts["s"] = True
            elif opt.startswith("br"):
                if len(opt) > 2:
                    try:
                        opts["br"] = int(opt[2:])
                    except:
                        pass
            elif opt.startswith("T"):
                if len(opt) > 1:
                    try:
                        opts["T"] = int(opt[1:])
                    except:
                        pass
            else:
                filenames.append(arg)
        else:
            filenames.append(arg)
    if twmet:
        opts["tw"] = True
    
    debug("procargs: opts == %s" % (repr(opts)))



def usage():
    debug("in usage")
    print """Usage: %s [-c] [-s] [-brN] [-TN] [-tw] [-tf1] [-tf2] [-hw] [-hf1] [-hf2] file1.jpg file2.gif ...
    Uploads images to imageshack.us and prints out HTML response to stdout.
        -c     -- output an HTML comment with uploaded file's name before each set of links
        -s     -- turn off stripping of useless crap (Post this image... etc)
        -brN   -- output N <br> tags after each link
        -TN    -- output an HTML table with N image links per row
        -tw    -- output 'Thumbnail for Websites' link, default if no other link options specified
        -tf1   -- output 'Thumbnail for Forums 1' link
        -tf2   -- output 'Thumbnail for Forums 2' link
        -hw    -- output 'Hotlink for Websites' link
        -hf1   -- output 'Hotlink for Forums 1' link
        -hf2   -- output 'Hotlink for Forums 2' link""" % (sys.argv[0])
    sys.exit(0)



def entitydecode(s):
    debug("in entitydecode")
    d = {
        "&gt;" : ">",
        "&lt;" : "<",
        "&quot;" : "\"",
    }
    debug("entitydecode: d == %s" % (repr(d)))
    debug("entitydecode: before ****\n%s" % (s))
    for (ent, subst) in d.iteritems():
        pos = s.find(ent)
        while pos != -1:
            news = s[:pos] + subst + s[pos+len(ent):]
            s = news
            pos = s.find(ent)
    debug("entitydecode: after ****\n%s" % (s))
    
    return s



def extractinputval(html):
    debug("in extractinputval")
    valpos = html.find("value=")
    if valpos == -1:
        return None
    start = valpos + len("value=") + 1
    end = html.find("\"", start)
    return html[start:end]



def findinput(s, seq=0):
    debug("in findinput")
    if seq < 0:
        return -1
    pos = s.find("<input")
    while pos != -1 and seq > 0:
        seq = seq - 1
        pos = s.find("<input", pos + len("<input"))
    return pos



def extractlink(htmlresp, linktype):
    debug("in extractlink")
    debug("extractlink: linktype == %s" % linktype)
    if linktype in ("tw", "tf1", "tf2"):
        return extract_thumbnail_link(htmlresp, linktype)
    elif linktype in ("hw", "hf1", "hf2"):
        return extract_hot_link(htmlresp, linktype)
    else:
        return "<!-- ERROR, extractlink(): unknown link type \"%s\"! -->" % (str(linktype))



def extract_thumbnail_link(htmlresp, linktype):
    debug("in extract_thumbnail_link")
    
    linkseq = {
        "tw" : 0,
        "tf1" : 1,
        "tf2" : 2
    }
    inpseq = linkseq[linktype]

    lines = htmlresp.split("\n")
    
    for line in lines:
        if line.find("Thumbnail for Websites") == -1:
            continue
        inputpos = findinput(line, inpseq)
        encodedlink = extractinputval(line[inputpos:])
        link = entitydecode(encodedlink)
        if opts["s"]:
            return link
        else:
            pos = link.find("<br />")
            if pos == -1:
                strippedlink = link
            else:
                strippedlink = link[:pos]
            return strippedlink



def extract_hot_link(htmlresp, linktype):
    debug("in extract_hot_link")
    d = {
        "hw" : "Hotlink for Websites",
        "hf1" : "Hotlink for forums (1)",
        "hf2" : "Hotlink for forums (2)"
    }
    searchstr = d[linktype]
    errmsg = "<!-- %s not found -->" % d[linktype]
    lines = htmlresp.split("\n")
    
    for i in range(0, len(lines)):
        if lines[i].find(searchstr) != -1:
            break
    if i == len(lines):
        return errmsg
    while i >= 0:
        i = i - 1
        s = lines[i]
        inppos = findinput(s, 0)
        if inppos == -1:
            continue
        val = extractinputval(s[inppos:])
        return entitydecode(val)
    return errmsg



def procresp(htmlresp, filename):
    global opts, links
    debug("in procresp")

	
    if htmlresp == None or len(htmlresp) == 0:
        if opts["c"]:
            links.append("<!-- %s -->\n" % (filename) + "<!-- ERROR, server returned 0 length response! -->")
        else:
            links.append("<!-- ERROR, %s upload failed, server returned 0 length response! -->" % (filename))
        return
    fname = ""
    for (key, val) in opts.iteritems():
        if key in ("c", "s", "br", "T") or val == False:
            continue
        if opts["c"]:
            fname = "<!-- %s -->\n" % (filename)
        links.append(str(fname) + str(extractlink(htmlresp, key)) + opts["br"] * brstr + "\n")



def uploadpics():
    debug("in uploadpic")
    if len(filenames) == 0:
        usage()
    for filename in filenames:
        try:
            f = open(filename, "rb")
            fdata = f.read()
        except:
            sys.stderr.write("%s: couldn't open file \"%s\"\n" % (sys.argv[0], filename))
            continue
        f.close()
        htmlresp = post_multipart(host, port, actionurl, [], [(fieldname, filename, fdata)])
        print("HTTP RESPONSE:\n")
        print(htmlresp + "\n\n")
        procresp(htmlresp, filename)



def printlinks():
    debug("in printlinks")
    
    if opts["T"] == 0:
        for link in links:
            print link
    else:
        print "<table>\n"
        i = 0
        for link in links:
            if (i % opts["T"]) == 0:
                print "<tr>"
            print "<td>\n" + link + "</td>"
            i = i + 1
            if (i % opts["T"]) == 0:
                print "</tr>\n"
        if (i % opts["T"]) != 0:
            while (i % opts["T"]) != 0:
                print "<td>\n&nbsp;\n</td>"
                i = i + 1
            print "</tr>"
        print "</table>"
                



### MAIN

procargs()
uploadpics()
printlinks()
