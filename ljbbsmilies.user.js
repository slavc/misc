// ==UserScript==
// @name           ljbbsmilies
// @namespace      ljbbsm
// @description    Adds smilies from phpBB to LiveJournal
// @include        http://*.livejournal.com/*
// ==/UserScript==

// Possible values are:
//  - phpbb
//  - skype
var smiley_set = "phpbb";


/**********************************************************************/
var g_enabled;

const FIREFOX = 0;
const OPERA = 1;
var g_browser = window.navigator.appName == 'Netscape' ? FIREFOX : OPERA;

var g_body = document.body;
var g_textarea = null;
var g_toolbar = null;
var g_formsubmit = null;

var g_smilies_menu;
var g_button;
var g_old_formsubmit_click;
var g_orig_quickreply;

function find_toolbar() {
    var div = document.getElementById('htmltools');
    if (!div)
        return null;
    var ul = div.getElementsByTagName('ul')[0];
    var li = document.createElement('li');
    ul.appendChild(li);

    return li;
}

function setup_smilies() {
    get_important_elements();

    if (!g_formsubmit) {
        return 0;
    }
    
    
    g_toolbar.appendChild(g_button.node);
    
    g_body.addEventListener('click', hide_smilies_menu, false);
    
    g_button.node.addEventListener('click', function(event) { show_smilies_menu(event); event.stopPropagation(); g_textarea.focus(); }, false);
    g_smilies_menu.node.addEventListener('click', hide_smilies_menu, false);
    
    if (g_browser == OPERA) {
        g_old_formsubmit_click = g_formsubmit.onclick;
        g_formsubmit.addEventListener('click', my_formsubmit_click, false);
    } else {
        g_formsubmit.addEventListener('click', replace_smilies, false);
    }
}

function my_formsubmit_click(event) {
    replace_smilies(event);
    g_old_formsubmit_click(event);
}

function get_important_elements() {
    var form;

    if (form = document.getElementById('updateForm')) {
        g_body = document.getElementsByTagName('body')[0];
        g_textarea = document.getElementsByTagName('textarea')[0];
        g_toolbar = find_toolbar();
        if (!(g_formsubmit = document.getElementById('formsubmit'))) {
            // maybe it's an update journal page then?
            var els = document.getElementsByName('action:save');
            g_formsubmit = els[els.length - 1];
        }
    } else if (form = document.getElementById('postform')) {
        var i;
        g_textarea = document.getElementById('commenttext');
    
        var div = document.createElement('div');
        g_toolbar = document.createElement('span');
        div.appendChild(g_toolbar);
        document.getElementById('userpics').appendChild(div);
        
        var els = form.getElementsByTagName('input');
        for (i = 0; i < els.length; ++i) {
            if (els[i].name == 'submitpost') {
                g_formsubmit = els[i];
                break;
            }
        }
    } else if (form = document.getElementById('qrform')) {
        g_textarea = document.getElementById('body');
        g_toolbar = document.createElement('span');
        document.getElementById('subject').parentNode.appendChild(g_toolbar);
        g_formsubmit = document.getElementById('submitpost');
    }
}

/*
// debug
window.addEventListener(
    'scroll',
    function (event) {
        dump('--- SCROLL ---\n');
        print_win_sizes();
    },
    false
);

document.body.addEventListener(
    'click',
    function (event) {
        dump('click.event.clientX: ' + event.clientX + '\n' + 'click.event.clientY: ' + event.clientY + '\n');
    },
    false
);

function print_win_sizes() {
    var vars = [
        'innerHeight',
        'innerWidth',
        'mozInnerScreenX',
        'mozInnerScreenY',
        'outerWidth',
        'outerHeight',
        'pageXOffset',
        'pageYOffset',
        'screenX',
        'screenY',
        'scrollMaxX',
        'scrollMaxY',
        'scrollX',
        'scrollY'
    ];
    var i, varname;

    for (i in vars) {
        varname = vars[i];
        dump('window.' + varname + ': ' + window[varname] + '\n');
    }
}
*/

function show() {
    var x, y, w, h, w_width, r_border, b_border, w_height;
    const gap = 4;

    this.node.style.visibility = 'visible';

    w_width = window.innerWidth - 30;
    x = parseInt(this.node.style.left);
    w = this.node.clientWidth;
    r_border = x + w;
    if (r_border > w_width) {
        x = x - (r_border - w_width) - gap;
        if (x < 0)
            x = 0;
        this.node.style.left = x + 'px';
    }

    w_height = window.innerHeight - 30;
    y = parseInt(this.node.style.top);
    h = this.node.clientHeight;
    b_border = y + h;
    if (b_border > w_height) {
        y = y - (b_border - w_height) - gap;
        if (y < 0)
            y = 0;
        this.node.style.top = y + 'px';
    }

    return this;
}
function moveTo(x, y) {
    this.node.style.left = x + 'px';
    this.node.style.top  = y + 'px';
    return this;
}
function SmiliesMenu() {
    this.node = document.createElement('div');
    this.node.style.position = 'fixed';
    this.node.style.top = '0px';
    this.node.style.left = '0xp';
    this.node.style.padding = '8px';
    this.node.style.width = '350px';
    this.node.style.borderColor = 'black';
    this.node.style.borderStyle = 'solid';
    this.node.style.borderWidth = '1px';
    this.node.style.visibility = 'hidden';
    this.node.style.backgroundColor = 'white';
    this.node.style.zOrder = 9999;

    if (smiley_set == "phpbb") {
        this.smilies = {
            ":XD:" : "http://img442.imageshack.us/img442/8296/44959742.gif",
            ":angry:" : "http://img406.imageshack.us/img406/8637/angryoh.gif",
            ":biggrin:" : "http://img143.imageshack.us/img143/864/biggrino.gif",
            ":bigsmile:" : "http://img203.imageshack.us/img203/5720/bigsmile.gif",
            ":blink:" : "http://img62.imageshack.us/img62/1577/blinkk.gif",
            ":blush:" : "http://img42.imageshack.us/img42/9793/blushc.gif",
            ":butbut:" : "http://img25.imageshack.us/img25/4449/butbut.gif",
            ":confused:" : "http://img11.imageshack.us/img11/9729/confusedu.gif",
            ":cool:" : "http://img689.imageshack.us/img689/4875/coolhf.gif",
            ":crazy:" : "http://img687.imageshack.us/img687/1574/crazyu.gif",
            ":cry:" : "http://img594.imageshack.us/img594/3525/cryo.gif",
            ":crying:" : "http://img521.imageshack.us/img521/2185/crying.gif",
            ":devil:" : "http://img411.imageshack.us/img411/706/deviln.gif",
            ":eviltongue:" : "http://img504.imageshack.us/img504/7766/eviltongue.gif",
            ":excl:" : "http://img291.imageshack.us/img291/5944/excl.gif",
            ":fear:" : "http://img442.imageshack.us/img442/7779/fear.gif",
            ":flowers:" : "http://img406.imageshack.us/img406/3815/flowerse.gif",
            ":geek:" : "http://img143.imageshack.us/img143/3356/geekh.gif",
            ":glare:" : "http://img203.imageshack.us/img203/441/glare.gif",
            ":happy:" : "http://img62.imageshack.us/img62/4039/happyh.gif",
            ":heart:" : "http://img42.imageshack.us/img42/8913/heartnw.gif",
            ":hehe:" : "http://img25.imageshack.us/img25/7854/hehef.gif",
            ":hmmm:" : "http://img11.imageshack.us/img11/7103/hmmmv.gif",
            ":huh:" : "http://img689.imageshack.us/img689/8143/huhkm.gif",
            ":innocent:" : "http://img411.imageshack.us/img411/1730/innocent.gif",
            ":kiss:" : "http://img504.imageshack.us/img504/8940/kissbe.gif",
            ":laugh:" : "http://img291.imageshack.us/img291/9388/laughu.gif",
            ":mellow:" : "http://img203.imageshack.us/img203/5293/mellowz.gif",
            ":music:" : "http://img62.imageshack.us/img62/2146/musics.gif",
            ":notworthy:" : "http://img42.imageshack.us/img42/1948/notworthyl.gif",
            ":ohmy:" : "http://img25.imageshack.us/img25/7503/ohmyr.gif",
            ":rolleyes:" : "http://img11.imageshack.us/img11/4791/rolleyesi.gif",
            ":sad:" : "http://img689.imageshack.us/img689/8214/sadd.gif",
            ":sad2:" : "http://img687.imageshack.us/img687/9141/sad2.gif",
            ":shifty:" : "http://img594.imageshack.us/img594/7150/shifty.gif",
            ":sleeping:" : "http://img521.imageshack.us/img521/1821/sleeping.gif",
            ":sly:" : "http://img402.imageshack.us/img402/9042/slym.gif",
            ":smile:" : "http://img179.imageshack.us/img179/3749/smilej.gif",
            ":smiling:" : "http://img153.imageshack.us/img153/418/smiling.gif",
            ":thumbs-up:" : "http://img100.imageshack.us/img100/8426/thumbsup.gif",
            ":tongue:" : "http://img693.imageshack.us/img693/4546/tongueex.gif",
            ":unsure:" : "http://img203.imageshack.us/img203/7646/unsurev.gif",
            ":w00t:" : "http://img535.imageshack.us/img535/6628/w00t.gif",
            ":wacko:" : "http://img534.imageshack.us/img534/6413/wackok.gif",
            ":whistling:" : "http://img130.imageshack.us/img130/8662/whistlingh.gif",
            ":wink:" : "http://img63.imageshack.us/img63/216/winku.gif",
            ":wub:" : "http://img718.imageshack.us/img718/3589/wubh.gif",
            ":xmas:" : "http://img682.imageshack.us/img682/3183/xmasu.gif",
            ":yin-yang:" : "http://img532.imageshack.us/img532/3685/yinyang.gif",
        };
        this.smilie_aliases = {
            ":-)" : ":smile:",
            ":-D" : ":biggrin:",
            ";-)" : ":wink:",
            "x-D" : ":XD:",
            "X-D" : ":XD:",
            ":-O" : ":ohmy:",
            ":-(" : ":sad:",
            "o_O" : ":blink:",
            "o_o" : ":blink:",
            "O_o" : ":blink:",
            "D-:" : ":crazy:",
            ":-/" : ":confused:",
            ";_;" : ":crying:",
            "<3"  : ":heart:",
            "(!)" : ":excl:"
        };
    } else if (smiley_set == "skype") {
        this.smilies = {
            "(sadsmile)" : "http://img215.imageshack.us/img215/9917/skypeemoticons01sadsmil.gif",
            "(bigsmile)" : "http://img693.imageshack.us/img693/4186/skypeemoticons02bigsmil.gif",
            "(cool)" : "http://img685.imageshack.us/img685/9418/skypeemoticons03cool.gif",
            "(wink)" : "http://img59.imageshack.us/img59/136/skypeemoticons05wink.gif",
            "(crying)" : "http://img51.imageshack.us/img51/4746/skypeemoticons06crying.gif",
            "(sweating)" : "http://img25.imageshack.us/img25/7151/skypeemoticons07sweatin.gif",
            "(speechless)" : "http://img18.imageshack.us/img18/5986/skypeemoticons08speechl.gif",
            "(kiss)" : "http://img718.imageshack.us/img718/2397/skypeemoticons09kiss.gif",
            "(tongueout)" : "http://img521.imageshack.us/img521/233/skypeemoticons10tongueo.gif",
            "(blush)" : "http://img511.imageshack.us/img511/8496/skypeemoticons11blush.gif",
            "(wondering)" : "http://img249.imageshack.us/img249/4679/skypeemoticons12wonderi.gif",
            "(sleepy)" : "http://img404.imageshack.us/img404/3351/skypeemoticons13sleepy.gif",
            "(dull)" : "http://img72.imageshack.us/img72/9681/skypeemoticons14dull.gif",
            "(inlove)" : "http://img293.imageshack.us/img293/7858/skypeemoticons15inlove.gif",
            "(evilgrin)" : "http://img25.imageshack.us/img25/9413/skypeemoticons16evilgri.gif",
            "(talking)" : "http://img88.imageshack.us/img88/766/skypeemoticons17talking.gif",
            "(yawn)" : "http://img215.imageshack.us/img215/986/skypeemoticons18yawn.gif",
            "(puke)" : "http://img693.imageshack.us/img693/2752/skypeemoticons19puke.gif",
            "(doh)" : "http://img685.imageshack.us/img685/105/skypeemoticons20doh.gif",
            "(angry)" : "http://img59.imageshack.us/img59/7536/skypeemoticons21angry.gif",
            "(itwasntme)" : "http://img51.imageshack.us/img51/2672/skypeemoticons22itwasnt.gif",
            "(party)" : "http://img25.imageshack.us/img25/7987/skypeemoticons23party.gif",
            "(worried)" : "http://img18.imageshack.us/img18/2102/skypeemoticons24worried.gif",
            "(mmm)" : "http://img718.imageshack.us/img718/5066/skypeemoticons25mmm.gif",
            "(nerd)" : "http://img521.imageshack.us/img521/581/skypeemoticons26nerd.gif",
            "(lipssealed)" : "http://img511.imageshack.us/img511/5451/skypeemoticons27lipssea.gif",
            "(hi)" : "http://img249.imageshack.us/img249/617/skypeemoticons28hi.gif",
            "(call)" : "http://img404.imageshack.us/img404/8548/skypeemoticons29call.gif",
            "(devil)" : "http://img72.imageshack.us/img72/8805/skypeemoticons30devil.gif",
            "(angel)" : "http://img293.imageshack.us/img293/6785/skypeemoticons31angel.gif",
            "(envy)" : "http://img88.imageshack.us/img88/2596/skypeemoticons32envy.gif",
            "(wait)" : "http://img215.imageshack.us/img215/2520/skypeemoticons33wait.gif",
            "(bear)" : "http://img693.imageshack.us/img693/2928/skypeemoticons34bear.gif",
            "(makeup)" : "http://img685.imageshack.us/img685/3116/skypeemoticons35makeup.gif",
            "(giggle)" : "http://img59.imageshack.us/img59/5732/skypeemoticons36giggle.gif",
            "(clapping)" : "http://img51.imageshack.us/img51/2725/skypeemoticons37clappin.gif",
            "(thinking)" : "http://img25.imageshack.us/img25/3874/skypeemoticons38thinkin.gif",
            "(bow)" : "http://img18.imageshack.us/img18/4066/skypeemoticons39bow.gif",
            "(rofl)" : "http://img718.imageshack.us/img718/151/skypeemoticons40rofl.gif",
            "(whew)" : "http://img88.imageshack.us/img88/8092/skypeemoticons41whew.gif",
            "(happy)" : "http://img18.imageshack.us/img18/4458/skypeemoticons42happy.gif",
            "(smirk)" : "http://img215.imageshack.us/img215/5690/skypeemoticons43smirk.gif",
            "(nod)" : "http://img693.imageshack.us/img693/8568/skypeemoticons44nod.gif",
            "(shake)" : "http://img685.imageshack.us/img685/8883/skypeemoticons45shake.gif",
            "(punch)" : "http://img59.imageshack.us/img59/9932/skypeemoticons46punch.gif",
            "(emo)" : "http://img51.imageshack.us/img51/7244/skypeemoticons47emo.gif",
            "(yes)" : "http://img25.imageshack.us/img25/900/skypeemoticons48yes.gif",
            "(no)" : "http://img18.imageshack.us/img18/605/skypeemoticons49no.gif",
            "(handshake)" : "http://img718.imageshack.us/img718/8674/skypeemoticons50handsha.gif",
            "(skype)" : "http://img521.imageshack.us/img521/7228/skypeemoticons51skype.gif",
            "(heart)" : "http://img511.imageshack.us/img511/863/skypeemoticons52heart.gif",
            "(brokenheart)" : "http://img249.imageshack.us/img249/5262/skypeemoticons53brokenh.gif",
            "(mail)" : "http://img404.imageshack.us/img404/1822/skypeemoticons54mail.gif",
            "(flower)" : "http://img72.imageshack.us/img72/9161/skypeemoticons55flower.gif",
            "(rain)" : "http://img293.imageshack.us/img293/1928/skypeemoticons56rain.gif",
            "(sun)" : "http://img88.imageshack.us/img88/3553/skypeemoticons57sun.gif",
            "(time)" : "http://img215.imageshack.us/img215/8866/skypeemoticons58time.gif",
            "(music)" : "http://img693.imageshack.us/img693/263/skypeemoticons59music.gif",
            "(movie)" : "http://img685.imageshack.us/img685/8636/skypeemoticons60movie.gif",
            "(phone)" : "http://img59.imageshack.us/img59/5674/skypeemoticons61phone.gif",
            "(coffee)" : "http://img51.imageshack.us/img51/8614/skypeemoticons62coffee.gif",
            "(pizza)" : "http://img25.imageshack.us/img25/4703/skypeemoticons63pizza.gif",
            "(cash)" : "http://img18.imageshack.us/img18/3629/skypeemoticons64cash.gif",
            "(muscle)" : "http://img718.imageshack.us/img718/263/skypeemoticons65muscle.gif",
            "(cake)" : "http://img521.imageshack.us/img521/5217/skypeemoticons66cake.gif",
            "(beer)" : "http://img511.imageshack.us/img511/966/skypeemoticons67beer.gif",
            "(drink)" : "http://img249.imageshack.us/img249/7611/skypeemoticons68drink.gif",
            "(dance)" : "http://img404.imageshack.us/img404/9527/skypeemoticons69dance.gif",
            "(ninja)" : "http://img293.imageshack.us/img293/3987/skypeemoticons70ninja.gif",
            "(star)" : "http://img88.imageshack.us/img88/2716/skypeemoticons71star.gif",
            "(mooning)" : "http://img404.imageshack.us/img404/2161/skypeemoticons72mooning.gif",
            "(middlefinger)" : "http://img72.imageshack.us/img72/3724/skypeemoticons73middlef.gif",
            "(bandit)" : "http://img293.imageshack.us/img293/9889/skypeemoticons74bandit.gif",
            "(drunk)" : "http://img88.imageshack.us/img88/3300/skypeemoticons75drunk.gif",
            "(smoke)" : "http://img215.imageshack.us/img215/9668/skypeemoticons76smoke.gif",
            "(toivo)" : "http://img693.imageshack.us/img693/5944/skypeemoticons77toivo.gif",
            "(rock)" : "http://img685.imageshack.us/img685/1015/skypeemoticons78rock.gif",
            "(headbang)" : "http://img59.imageshack.us/img59/6249/skypeemoticons79headban.gif",
            "(bug)" : "http://img51.imageshack.us/img51/5301/skypeemoticons80bug.gif",
            "(fubar)" : "http://img25.imageshack.us/img25/6026/skypeemoticons81fubar.gif",
            "(poolparty)" : "http://img18.imageshack.us/img18/6520/skypeemoticons82poolpar.gif",
            "(swear)" : "http://img718.imageshack.us/img718/2307/skypeemoticons83swear.gif",
            "(tmi)" : "http://img521.imageshack.us/img521/6575/skypeemoticons84tmi.gif",
            "(heidy)" : "http://img511.imageshack.us/img511/5670/skypeemoticons85heidy.gif",
            "(smile)" : "http://img249.imageshack.us/img249/6522/skypeemoticonssmile.gif",
        };
        this.smilie_aliases = {};
    } else {
        alert("Unrecognized smiley_set, edit the script");
    }
    
    document.getElementsByTagName('body')[0].appendChild(this.node);

    this.show = show;
    this.moveTo = moveTo;

    var div1, div2;

    div1 = document.createElement('div');
    div2 = document.createElement('div');

    for (var name in this.smilies) {
        var entry = create_smilie_entry(name, this.smilies[name], on_smilie_entry_click);
        div1.appendChild(entry);
    }

    var chkbox = document.createElement('input');
    chkbox.type = 'checkbox';
    chkbox.id = 'ljbbsmenabled';
    if (g_enabled)
        chkbox.checked = 'checked';
    chkbox.addEventListener('click', function(event) { if (g_enabled) { g_enabled = false; hide_smilies_menu(); } else g_enabled = true; GM_setValue('ljbbsmilies.enabled', g_enabled); event.stopPropagation(); }, false);
    var label = document.createElement('label');
    label.setAttribute('for', 'ljbbsmenabled');
    label.textContent = 'Enable smilies';
    var hr = document.createElement('hr');
    hr.width = '1';
    hr.noshade = 'noshade';

    div2.appendChild(chkbox);
    div2.appendChild(label);

    this.node.appendChild(div1);
    if (g_browser == FIREFOX) {
        this.node.appendChild(hr);
        this.node.appendChild(div2);
    }

    return this;
}

function get_caret_pos(node) {
    pos = 0;
    if (node.selectionStart || node.selectionStart == '0')
        pos = node.selectionStart;
    return pos;
}

function on_smilie_entry_click(event) {
    var el = event.target;
    var smiley;
    var pos;

    if (!g_enabled)
        return false;

    g_textarea.focus();

    smiley = ' ' + el.alt + ' ';

    insert_text(g_textarea, smiley, true);

    return false;
}

/* This is a modified function taken from phpBB */
function insert_text(textarea, text)
{
    var sel_start = textarea.selectionStart;
    var sel_end = textarea.selectionEnd;
    
    mozWrap(textarea, text, '');
    textarea.selectionStart = sel_start + text.length;
    textarea.selectionEnd = sel_end + text.length;
}

/**
* From http://www.massless.org/mozedit/
*/
function mozWrap(txtarea, open, close)
{
    var selLength = (typeof(txtarea.textLength) == 'undefined') ? txtarea.value.length : txtarea.textLength;
    var selStart = txtarea.selectionStart;
    var selEnd = txtarea.selectionEnd;
    var scrollTop = txtarea.scrollTop;

    if (selEnd == 1 || selEnd == 2) 
    {
        selEnd = selLength;
    }

    var s1 = (txtarea.value).substring(0,selStart);
    var s2 = (txtarea.value).substring(selStart, selEnd)
    var s3 = (txtarea.value).substring(selEnd, selLength);

    txtarea.value = s1 + open + s2 + close + s3;
    txtarea.selectionStart = selStart + open.length;
    txtarea.selectionEnd = selEnd + open.length;
    txtarea.focus();
    txtarea.scrollTop = scrollTop;

    return;
}

function create_smilie_entry(name, imglink, onclick_func) {
    var a, img;

    a = document.createElement('a');
    a.href = 'javascript:void(0);';
    a.addEventListener('click', onclick_func, false);

    img = document.createElement('img');
    img.src = imglink;
    img.title = name;
    img.alt = name;
    img.border = '0';
    img.style.margin = '1px';

    a.appendChild(img);

    return a;
}

function replace_smilies(event) {
    var txt = g_textarea.value, pat, alt;

    if (!g_enabled)
        return;

    for (var name in g_smilies_menu.smilies) {
        alt = name.substring(1, name.length - 1);
        pat = name;
        while (txt.indexOf(pat) != -1)
            txt = txt.replace(pat, '<img alt="' + alt + '" src="' + g_smilies_menu.smilies[name] + '">');
    }
    for (var name in g_smilies_menu.smilie_aliases) {
        alt = '';
        pat = name;
        while (txt.indexOf(pat) != -1)
            txt = txt.replace(pat, '<img alt="' + alt + '" src="' + g_smilies_menu.smilies[g_smilies_menu.smilie_aliases[name]] + '">');
    }
    g_textarea.value = txt;
}

function SmiliesButton() {
    var a, img;

    a = document.createElement('a');
    this.node = a;
    a.href = 'javascript:void(0);';

    img = document.createElement('img');
    img.border = '0';
    img.src = 'http://img717.imageshack.us/img717/2755/biggrina.gif';

    a.appendChild(img);

    return this;
}

function show_smilies_menu(event) {
    var x, y;
    x = event.clientX + event.target.clientWidth / 2;
    y = event.clientY + event.target.clientHeight / 2;
    g_smilies_menu.moveTo(x, y);
    g_smilies_menu.show();
}

function hide_smilies_menu(event) {
    g_smilies_menu.node.style.visibility = 'hidden';
}



// MAIN

if (g_browser == FIREFOX)
    g_enabled = GM_getValue('ljbbsmilies.enabled', true);
else
    g_enabled = true;

g_smilies_menu = new SmiliesMenu();
g_button = new SmiliesButton();

if (g_browser == FIREFOX)
    g_orig_quickreply = unsafeWindow.quickreply;
else
    g_orig_quickreply = window.quickreply;

function myquickreply(a, b, c) {
    g_orig_quickreply(a, b, c);
    setup_smilies();
    return false;
}

if (g_browser == FIREFOX)
    unsafeWindow.quickreply = myquickreply;
else
    window.quickreply = myquickreply;

setup_smilies();
