default:
	cmake -B build/
	make -C build/
	go build -o bin/pir go/main.go go/client.go go/database.go go/protocol.go go/recover.go go/server.go go/utils.go
	go build -o bin/bm_pir go/benchmark.go go/client.go go/database.go go/protocol.go go/recover.go go/server.go go/utils.go

test:
	./bin/tests
	(cd go; go test)
