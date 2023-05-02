PicoWi project, see iosoft.blog/picowi for details
To create makefile, run
  cmake ..

Then make PicoWi library:
  make picowi

Then make application, e.g.:
  make blink

To program a Pico using OpenOCD bit-bashed SWD:
  chmod +x prog
  ./prog blink
