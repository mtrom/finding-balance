default:
	cmake -B build/
	make -C build/
	(cd go; go build -o ../bin/pir)

test:
	./bin/tests
	(cd go; go test)
	./bin/correctness.sh
