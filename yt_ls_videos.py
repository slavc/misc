import sys, urllib2, xml.parsers.expat
from string import join

MAX_RESULTS = 50 # the maximum n of records that can be
                 # retrieved in one HTTP request as defined
                 # by YouTube API

video_list = []
tag_stack = []
tag_stack_str = ''
video_info = None

class VideoInfo:
    def __init__(self):
        self.tag2prop = {
            'entry/id' : {
                'attrs' : False,
                'dataattr' : False,
                'prop' : 'vid',
            },
            'entry/published' : {
                'attrs' : False,
                'dataattr' : False,
                'prop' : 'data_published',
            },
            'entry/link' : {
                'attrs' : {
                    'rel' : 'alternate',
                    'type' : 'text/html',
                },
                'dataattr' : 'href',
                'prop' : 'url',
            },
            'entry/media:group/media:title' : {
                'attrs' : False,
                'dataattr' : False,
                'prop': 'title',
            },
            'entry/media:group/media:description' : {
                'attrs' : False,
                'dataattr' : False,
                'prop': 'descr',
            },
        }

        self.vid = ''
        self.title = ''
        self.descr = ''
        self.url = ''
        self.date_published = ''
    def __cmp__(self, d):
        if isinstance(d, VideoInfo):
            return cmp(self.title, d.title)
        else:
            return 0

def push_tag(tagname):
    global tag_stack, tag_stack_str

    tag_stack.append(tagname)
    tag_stack_str = join(tag_stack, '/')

def pop_tag():
    global tag_stack, tag_stack_str

    tagname = tag_stack[-1]
    tag_stack = tag_stack[:-1]
    tag_stack_str = join(tag_stack, '/')
    return tagname

def StartElementHandler(tag, attrs):
    global tag_stack, tag_stack_str, video_info

    push_tag(tag)

    if tag == 'entry':
        video_info = VideoInfo()
    elif video_info:
        for taghier in video_info.tag2prop:
            if tag_stack_str.endswith(taghier):
                checks_passed = True
                if video_info.tag2prop[taghier]['attrs']:
                    chkattrs = video_info.tag2prop[taghier]['attrs']
                    for attr in chkattrs:
                        if attrs.get(attr, '') != chkattrs.get(attr):
                            checks_passed = False
                            break
                if checks_passed:
                    if video_info.tag2prop[taghier]['dataattr']:
                        data = attrs.get(video_info.tag2prop[taghier]['dataattr'], '')
                        prop = video_info.tag2prop[taghier]['prop']
                        setattr(video_info, prop, data)

def EndElementHandler(tag):
    global video_list, tag_stack, tag_stack_str, video_info

    pop_tag()

    if tag == 'entry':
        video_list.append(video_info)
        video_info = None

def escape_special_chars(s):
    s = s.replace('&', '&amp;')
    s = s.replace('<', '&lt;')
    s = s.replace('>', '&gt;')
    return s

def CharacterDataHandler(data):
    global video_info, tag_stack

    if not video_info:
        return

    tag = tag_stack[-1]
    for taghier in video_info.tag2prop:
        if tag_stack_str.endswith(taghier) and not video_info.tag2prop[taghier]['dataattr']:
            prop = video_info.tag2prop[taghier]['prop']
            old_data = getattr(video_info, prop, '')
            data = escape_special_chars(data)
            setattr(video_info, prop, old_data + data)

def parse_resp(xmldata):
    global tag_stack, video_info

    tag_stack = []
    video_info = None

    old_video_list_len = len(video_list)

    xmlp = xml.parsers.expat.ParserCreate()
    xmlp.StartElementHandler = StartElementHandler
    xmlp.EndElementHandler = EndElementHandler
    xmlp.CharacterDataHandler = CharacterDataHandler
    xmlp.Parse(xmldata, True)

    n_results = len(video_list) - old_video_list_len

    return n_results

def yt_get_video_list(channel_name):
    global video_list

    video_list = []

    start_index = 1
    max_results = MAX_RESULTS
    i = 0
    while True:
        url = 'https://gdata.youtube.com/feeds/api/users/%s/uploads?start-index=%s&max-results=%s' % (channel_name, start_index, max_results)
        resp = urllib2.urlopen(url)
        xmldata = resp.read()
        """
        f = open('resp%03i.xml' % i, 'w')
        f.write(xmldata)
        f.close()
        """
        n_results = parse_resp(xmldata)
        if n_results < max_results:
            break
        start_index += max_results
        i += 1

    return video_list

def yt_print_video_list(video_list):
    for video_info in video_list:
        buf = '%s\t%s\t%s' % (video_info.title, video_info.url, video_info.descr)
        print buf.encode('utf-8')

def gen_html(video_list, yt_user_name):
    buf = ''

    buf += """<head>
    <meta charset="utf-8">
    <title>Videos for %s</title>
    <style>
        a.title {
            font-size: small;
            font-weight: bold;
        }
        p.descr {
            font-size: x-small;
            white-space: pre-wrap;
        }
    </style>
</head>
<body>
    <header>
        <hgroup>
        <h1>Videos for %s</h1>
        </hgroup>
    </header>
    <div id="main">\n""" % (yt_user_name, yt_user_name)

    for v in video_list:
        buf += """        <div>
            <a class="title" target="_blank" href="%s">%s</a>
            <p class="descr">%s</p>
        </div>\n""" % (v.url, v.title, v.descr)

    buf += """    </div>
</body>
</html>"""

    return buf

if __name__ == '__main__':
    for channel_name in sys.argv[1:]:
        video_list = yt_get_video_list(channel_name)
        video_list.sort()
        buf = gen_html(video_list, channel_name)
        print buf.encode('utf-8')
