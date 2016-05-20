default: cleanbuildemu

clean:
	pebble clean


build:
	pebble build


cleanbuild:
	pebble clean; pebble build


cleanbuildemu:
	pebble clean; pebble build; pebble install -vv --emulator basalt


emuchalc:
	pebble install --emulator chalk
