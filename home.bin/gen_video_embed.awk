#!/usr/bin/awk -f

{
    fmt =     "<OBJECT ID=\"MediaPlayer\" WIDTH=\"640\" HEIGHT=\"480\" CLASSID=\"CLSID:22D6F312-B0F6-11D0-94AB-0080C74C7E95\" STANDBY=\"Loading Windows Media Player components...\" TYPE=\"application/x-oleobject\">";
    fmt = fmt "<PARAM NAME=\"FileName\" VALUE=\"%s\">";
    fmt = fmt "<PARAM name=\"autostart\" VALUE=\"false\">";
    fmt = fmt "<PARAM name=\"ShowControls\" VALUE=\"true\">";
    fmt = fmt "<param name=\"ShowStatusBar\" value=\"false\">";
    fmt = fmt "<PARAM name=\"ShowDisplay\" VALUE=\"false\">";
    fmt = fmt "<EMBED TYPE=\"application/x-mplayer2\" SRC=\"%s\" NAME=\"MediaPlayer\" WIDTH=\"640\" HEIGHT=\"480\" ShowControls=\"1\" ShowStatusBar=\"0\" ShowDisplay=\"0\" autostart=\"0\"> </EMBED>";
    fmt = fmt "</OBJECT>\n";

    printf(fmt, $0, $0);
}
