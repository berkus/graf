.SILENT

COMPILE = wpp386 -bt=dos -5r -fp3 -fpi87 -mf -xs -oneatx -d3s -I../H
OBJP = \Devel\OBJ\FINAL

obj1 = enfonted.obj
obj2 = biosfont.obj
obj3 = framrate.obj

enfonted.exe: $obj1
    echo Linking EnFonted
    wlink sys zrdx name EnFonted f {$obj1}

biosfont.exe: $obj2
    echo Linking BiosFont
    wlink sys pmodew name BiosFont f {$obj2}

framrate.exe: $obj3
    echo Linking FramRate
    wlink sys zrdx name FramRate f {$obj3}

all : enfonted.exe biosfont.exe framrate.exe .SYMBOLIC

enfonted.obj: enfonted.cpp
    $COMPILE enfonted.cpp -I../../KAOS/H;../../SYS

biosfont.obj: biosfont.cpp
    $COMPILE biosfont.cpp

framrate.obj: framrate.cpp
    $COMPILE framrate.cpp
