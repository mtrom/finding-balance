default:
	cmake -B build/
	make -C build/
	go build -o bin/pir go/*.go
	mkdir out
	mkdir logs

test:
	./bin/tests
	(cd go; go test)
	./bin/correctness.sh
