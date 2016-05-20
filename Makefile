default: cleanbuildemu

clean:
	pebble clean


build:
	pebble build


cleanbuild:
	pebble clean; pebble build


cleanbuildemu:
	pebble clean; pebble build; pebble install --emulator basalt


emuchalk:
	pebble clean; pebble build; pebble install --emulator chalk
