default:
	cmake -B build/
	make -C build/
	go build -o bin/pir go/*.go
