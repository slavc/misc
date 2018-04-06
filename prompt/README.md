prompt
======
Draws a ruler-style command prompt.

Example:

```
$ export COLUMNS # prompt determines screen width using this env var
$ PS1='$(prompt -f blue -b white \w\ \j%\t)\n'
```

* `$()` -- substitutes expression with the output of command within
* `-f blue` -- set foreground (text) color to blue
* `-b white` -- set background color to 'white'
* `\w`, `\j`, `\t` and `\n` -- see Bash man, PROMPTING section
* `%` -- puts a spring between left and right parts

The result looks like this:

![Screenshot demonstrating various prompt colors and styles.](https://raw.githubusercontent.com/slavc/misc/master/prompt/screenshot.png)
