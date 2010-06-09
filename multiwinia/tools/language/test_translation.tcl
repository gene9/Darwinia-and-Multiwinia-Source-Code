#!/usr/bin/tclsh

# ===============================================
# proc register -- adds strings to the strings table
# ===============================================
proc register {x y} {
  set ::lang($x) $y
}


# ===============================================
# proc lookup -- looks up a string in the strings table
# ===============================================
proc lookup key {
  set key [string tolower $key]

  if { [info exists ::lang($key$::mode)] } {
    set x $::lang($key$::mode)
  } elseif { [info exists ::lang($key)] } {
    set x $::lang($key)
  } else {
    foreach m $::modes {
      if { [info exists ::lang($key$m)] } {
        if { $::warnings } {
          return "${::lt}[+ bold]WARNING:[+] $key$::mode (found $key$m)${::gt}"
        } else {
          return ""
        }
      }
    }
    return "${::lt}[+ bold]ERROR:[+] $key${::gt}"
  }

  if { ![catch {eval "set y \"$x\""}] } {
    return $y
  } else {
    return "[+ bold]ERROR:[+] $x"
  }
}


# ===============================================
# proc readStringsFile -- registers the contents of the named strings file
# -----------------------------------------------
# Current limitations: IFMODE cannot be nested.
# ===============================================
proc readStringsFile fname {
  if { [catch {set f [open $fname]}] } {
    puts "[+ red]!!![+] Failed to open file: $fname"
    return 0
  }
  set linenum 1
  while { [gets $f line] >= 0 } {
    if { [regexp -line {^(\w+)\s+([^\#]+)} $line _ key val] > 0 } {
      regsub -all {\[KEY(\w+)\]} $val {[KEY \1]} val
      regsub -all {\[(\w+)\]} $val {[INC \1]} val
      regsub -all {\[ (\w+)\]} $val {[INC_ \1]} val
      regsub -all {\[INC (ELSE|ENDIF)\]} $val {[\1]} val
      regsub -all {(\$|\{|\})} $val {\\\1} val
      regsub -all {\[IFMODE (\w+)\](.*)\[ELSE\](.*)\[ENDIF\]} $val {[MODE \1 {\2} {\3}]} val
      regsub -all {\[IFMODE (\w+)\](.*)\[ENDIF\]} $val {[MODE \1 {\2} {}]} val
      register [string tolower $key] $val
      set ::lines([string tolower $key]) $linenum
    }
    incr linenum
  }
  close $f
  return $linenum
}


# ===============================================
# Functions to decode strings
# ===============================================

if { ![info exists mode] } { set mode _kbd }
set modes {_kbd _xin}
proc mode_is m { string equal $::mode $m }
proc kbd {} { set ::mode _kbd }
proc xin {} { set ::mode _xin }

# Include one translation inside another
proc INC key { lookup $key }
# Same as INC, but adds a leading space
proc INC_ key {
  set x [INC $key]
  if { [string equal "" $x] } {
    return ""
  } else {
    return " $x"
  }
}
# Lookup a key description (requires input system to implement)
proc KEY key {
  # Call out to the input system
  if { [mode_is _kbd] } {
    return "\[KEY$key\]"
  } else {
    return "${::lt}[+ bold]ERROR:[+] KEY outside KEYBOARD mode${::gt}"
  }
}
# Run iconic help (requires help system to implement)
proc HELP key {
  # Call out to help system
  set topics {BASICS CAMERA UNITCREATION ENDTASK SQUAD ENGINEER OFFICER \
        OFFICERSECONDARY ARMOUR RADAR GUNTURRET SECONDARY UNITSELECTION}
  if { [lsearch -exact $topics $key] == -1 } {
    return "${::lt}[+ bold]ERROR:[+] HELP $key${::gt}"
  } else {
    return ""
  }
}
proc MODE {m t f} {
  if { [string equal $m KEYBOARD] } { set m _kbd }
  if { [string equal $m XINPUT] } { set m _xin }
  if { [mode_is $m] } { set x $t } else { set x $f }
  if { ![catch {eval "set y \"$x\""}] } {
    return $y
  } else {
    return "[+ bold]ERROR:[+] $x"
  }
}


# ===============================================
# List functions
# ===============================================

proc lhead lst { lindex $lst 0 }
proc ltail lst { lrange $lst 1 end }

proc lfilter {lst re} {
  set tmp {}
  foreach el $lst {
    if { [regexp $re $el] > 0 } {
      lappend tmp $el
    }
  }
  return $tmp
}

proc lfilternot {lst re} {
  set tmp {}
  foreach el $lst {
    if { ![regexp $re $el] } {
      lappend tmp $el
    }
  }
  return $tmp
}


# ===============================================
# Miscellany
# ===============================================

proc max {a b} {
  if { $a > $b } { return $a } else { return $b }
}

proc min {a b} {
  if { $a < $b } { return $a } else { return $b }
}


# ===============================================
# Output
# ===============================================

proc pause {} {
  puts -nonewline stderr "Press RETURN to continue... "
  #flush stdout
  gets stdin
  return
}

proc redirect_stdout fname {
  set ::outfile $fname
  if { [string equal $fname ""] } {
    if { [llength [info command puts.orig]] } {
      rename puts fputs
      rename puts.orig puts
    }
  } else {
    if { ![llength [info command puts.orig]] } {
      rename puts puts.orig
      rename fputs puts
    }
  }
}

proc fputs {args} {
  if { [llength $args] > 2 || \
       ( [llength $args] == 2 && \
         ![string equal [lindex $args 0] -nonewline] ) } {
    eval "puts.orig $args"
  } elseif { [llength $args] == 2 && \
               [string equal [lindex $args 0] -nonewline] } {
    set f [open $::outfile a+]
      eval "puts.orig -nonewline $f [lindex $args 1]"
    close $f
  } else {
    set f [open $::outfile a+]
      eval "puts.orig $f $args"
    close $f
  }
}


# ===============================================
# Command Line Arguments
# ===============================================

proc fixargv {} {
  if { [nextargv --] } { shiftargv }
}

proc shiftargv {} {
  set ::argv [ltail $::argv]
  set ::argc [llength $::argv]
}

proc nextargv x {
  string equal [lhead $::argv] $x
}

proc cutargv x {
  set idx [lsearch $::argv $x]
  if { $idx > -1 } {
    set ::argv [concat [lrange $::argv 0 [expr $idx-1]] [lrange $::argv [expr $idx+1] end]]
    set ::argc [llength $::argv]
    return 1
  } else {
    return 0
  }
}


# ===============================================
# ANSI colour output
# ===============================================

namespace eval ansi {
  namespace export + header footer lt gt

  variable colours {
    bold 1 light 2 blink 5 invert 7
    black 30 red 31 green 32 yellow 33 blue 34 purple 35 cyan 36 white 37
    Black 40 Red 41 Green 42 Yellow 43 Blue 44 Purple 45 Cyan 46 White 47
  }

  proc + {args} {
    variable colours
    set t 0
    foreach i $args {
      set ix [lsearch -exact $colours $i]
      if {$ix>-1} {lappend t [lindex $colours [incr ix]]}
    }
    return "\033\[[join $t {;}]m"
  }

  proc header {} {}
  proc footer {} {}

  variable lt "<"
  variable gt ">"
  variable endl ""
}


# ===============================================
# ANSI colour output
# ===============================================

namespace eval html {
  namespace export + header footer lt gt

  variable colours {
    bold {{<b>} {</b>}}
    black {{<span style="color: black;">} {</span>}}
    red {{<span style="color: red;">} {</span>}}
    green {{<span style="color: green;">} {</span>}}
    yellow {{<span style="color: #f60;">} {</span>}}
    blue {{<span style="color: blue;">} {</span>}}
    purple {{<span style="color: purple;">} {</span>}}
    cyan {{<span style="color: cyan;">} {</span>}}
    white {{<span style="color: white;">} {</span>}}
    Black {{<span style="background-color: black;">} {</span>}}
    Red {{<span style="background-color: red;">} {</span>}}
    Green {{<span style="background-color: green;">} {</span>}}
    Yellow {{<span style="background-color: yellow;">} {</span>}}
    Blue {{<span style="background-color: blue;">} {</span>}}
    Purple {{<span style="background-color: purple;">} {</span>}}
    Cyan {{<span style="background-color: cyan;">} {</span>}}
    White {{<span style="background-color: white;">} {</span>}}
  }

  variable stack {}


  proc + {args} {
    variable colours
    variable stack
    set t $stack
    set stack {}
    if { [llength $args] } {
      foreach i $args {
        set ix [lsearch -exact $colours $i]
        if {$ix>-1} {
          set x [lindex $colours [incr ix]]
          lappend t [lindex $x 0]
          set stack [linsert $stack 0 [lindex $x 1]]
        }
      }
    }
    return [join $t ""]
  }

  proc header {} {
    puts {<html><head><title>Translation Errors Report</title></head>}
	puts {<body style="font-family: monospace;">}
  }

  proc footer {} {
    puts {</body></html>}
  }

  variable lt "&lt;"
  variable gt "&gt;"
  variable endl "<br/>"
}

# ===============================================
# General colour output
# ===============================================

namespace eval nocolour {
  namespace export + header footer lt gt
  proc + args {}
  proc header {} {}
  proc footer {} {}
  variable lt "<"
  variable gt ">"
  variable endl ""
}

proc colour x {
  if { [llength [info command header]] } { rename header {} }
  if { [llength [info command footer]] } { rename footer {} }
  if { [llength [info command +]] } { rename + {} }
  namespace import ${x}::header
  namespace import ${x}::footer
  namespace import ${x}::+
  set ::lt [set ${x}::lt]
  set ::gt [set ${x}::gt]
  set ::endl [set ${x}::endl]
}

#if { ![llength [info command +]] } {
#  namespace import ansi::+
#}


# ===============================================
# Debugging and Testing
# ===============================================

if { ![info exists warnings] } {
  set warnings 0
}
proc warn b { set ::warnings $b }

if { ![info exists printparts] } {
  set printparts 0
}

proc reload {} {
  array unset ::lang
  array unset ::lines
  uplevel 1 source ./test.tcl 
}

proc basenames names {
  set names [lfilternot $names {^part_}]
  set tmp {}
  foreach name $names {
    regsub {(_kbd|_xin)$} $name {} name
    lappend tmp $name
  }
  set tmp [lsort $tmp]
  set lastName {}
  set tmp2 {}
  foreach name $tmp {
    if { ![string equal $name $lastName] } {
      lappend tmp2 $name
      set lastName $name
    }
  }
  return $tmp2
}

proc wrong_mode_suffix key {
  if { [regexp {(_[a-z]+)$} $key _ m] && \
       ([lsearch $::modes $m] >= 0) && \
       ![mode_is $m] } { return 1 }
  return 0
}

proc should_print key {
  if { ![wrong_mode_suffix $key] && \
       ( $::printparts || ![regexp {^part_} $key] ) } {
    return 1
  }
  return 0
}

proc findErrors {} {
  set errs {}
  #puts -nonewline stderr "[+ bold yellow]"
  foreach key [array names ::lang] {
    if { [should_print $key] } {
      set x [lookup $key]
      if { [regexp {ERROR:} $x] } {
        lappend errs [list $::lines($key) "$key (line [+ red]$::lines($key)[+]) --$::gt $x$::endl"]
      } elseif { $::warnings && [regexp {WARNING:} $x] } {
        lappend errs [list $::lines($key) "$key (line [+ yellow]$::lines($key)[+]) --$::gt $x$::endl"]
      } elseif { ![mode_is _kbd] } {
        if { [regexp {[Cc]lick|CTRL-C|TAB|SHIFT|[Mm]ouse|[Ff][1-9]|SPACE} $x ] } {
          lappend errs [list $::lines($key) "$key (line [+ yellow]$::lines($key)[+]) --$::gt $x$::endl"]
        }
      }
    }
    #puts -nonewline stderr "."
  }
  #puts stderr "[+]"
  foreach err [lsort -integer -index 0 $errs] {
    puts [lindex $err 1]
  }
}

proc pad {str len} {
  format "%-[set len]s" $str
}

# Print strings, keeping the line numbers in step with the source file
# Just leaves blank lines where there were comments
proc printAllStrings {} {
  set strs {}
  foreach key [array names ::lang] {
    if { [should_print $key] } {
      set x [lookup $key]
      lappend strs [list $::lines($key) "[+ bold blue][pad $key 37][+] $x$::endl"]
    }
  }
  set line 1
  foreach str [lsort -integer -index 0 $strs] {
    if { $::sync_lines } {
      while { $line < [lindex $str 0] } { puts "$::endl" ; incr line }
    }
    puts [lindex $str 1]
    incr line
  }
}

proc test {} {
  set tmp $::mode
  foreach m $::modes {
    puts "[+ bold green]\[ TESTING MODE: $m \][+]$::endl"
    set ::mode $m
    findErrors
  }
  puts "[+ bold green]\[ RESTORING MODE: $tmp \][+]$::endl"
  set ::mode $tmp
  return
}

proc compareWithEnglish {} {
  array set tmp [array get ::lang]
  array set tmp2 [array get ::lines]
  unset ::lang ::lines
  if { [readStringsFile english.txt] } {
    set engnames [basenames [array names ::lang]]
    set transnames [basenames [array names tmp]]
    set eng_idx 0
    set trans_idx 0
    while { ($eng_idx < [llength $engnames]) && \
            ($trans_idx < [llength $transnames]) } {
      set eng [lindex $engnames $eng_idx]
      set trans [lindex $transnames $trans_idx]
      set cmp [string compare $eng $trans]
      if { $cmp < 0 } {
        puts "Translation is missing string: [+ red]$eng[+]$::endl"
        incr eng_idx
      } elseif { $cmp > 0 } {
        puts "Translation has unknown string: [+ yellow]$trans[+]$::endl"
        incr trans_idx
      } else {
        incr eng_idx
        incr trans_idx
      }
    }
    while { $eng_idx < [llength $engnames] } {
      puts "Translation is missing string: [+ red][lindex $engnames $eng_idx][+]$::endl"
      incr eng_idx
    }
    while { $trans_idx < [llength $transnames] } {
      puts "Translation has unknown string: [+ yellow][lindex $transnames $trans_idx][+]$::endl"
      incr trans_idx
    }
  }
  if { [info exists ::lang] } { unset ::lang }
  if { [info exists ::lines] } { unset ::lines }
  array set ::lang [array get tmp]
  array set ::lines [array get tmp2]
}

proc setopmode op {
  if { [info exists ::opmode] } {
    puts "[+ yellow]\[ Setting operation to '$op' \][+]$::endl"
  }
  set ::opmode $op
}

proc setup_report {} {
  setopmode report
  colour html
  set ::do_pause 0
  if { [file isfile translation_test_report.html] } {
    file delete translation_test_report.html
  }
  redirect_stdout translation_test_report.html
}


# ===============================================
# MAIN PROGRAM
# ===============================================

proc print_usage {} {
  puts "Usage: [file tail $::argv0] \[-ansi|-html\] \[-test|-print|-compare|-report\]"
  puts "           \[-kbd|-xin\] \[-parts\] \[-nosynclines\] \[-warn\] \[-nopause\] \[-long\]"
}

if { ![info exists do_pause] } { set do_pause 1 }
if { ![info exists long_report] } { set long_report 0 }
if { ![llength [info command +]] } { colour nocolour }

# Deal with the command line arguments
if { ![info exists langfile] } {
  set langfile english.txt
  fixargv
  if { [cutargv -ansi] } { colour ansi }
  if { [cutargv -html] } { colour html }
  if { [cutargv --help] } { print_usage ; exit }
  if { [cutargv -test] } { setopmode test }
  if { [cutargv -print] } { setopmode print }
  if { [cutargv -compare] } { setopmode compare }
  if { [cutargv -parts] } { set printparts 1 }
  if { [cutargv -nosynclines] } { set sync_lines 0 }
  if { [cutargv -kbd] } { kbd }
  if { [cutargv -xin] } { xin }
  if { [cutargv -warn] } { warn 1 }
  if { [cutargv -nopause] } { set do_pause 0 }
  if { [cutargv -report] } { setup_report }
  if { [cutargv -long] } { set long_report 1 ; set sync_lines 0 }
  if { $argc > 0 } {
    set langfile [lhead $argv]
  }
}

if { ![info exists opmode] } { setup_report }
if { ![info exists sync_lines] } { set sync_lines 1 }


header

if { [readStringsFile $langfile] } {

  if { [string equal $opmode print] } {
    printAllStrings
  } elseif { [string equal $opmode test] } {
    test
  } elseif { [string equal $opmode compare] } {
    compareWithEnglish
  } elseif { [string equal $opmode report] } {
    puts "<h1>Error Report for [file tail $langfile]</h1>"
    puts "<h2>Looking for Errors in the Translation Strings</h2>"
    test
    puts "<h2>Looking for Omissions</h2>"
    compareWithEnglish
    if { $long_report } {
      puts "<h2>Listing the Expanded Strings</h2>"
      puts -nonewline "<pre>"
      printAllStrings
      puts "</pre>"
    }
  } else {
    puts "[+ red]\[ Unknown operation: $op\][+]"
  }

}

footer

if { $do_pause } { pause }
