all: gojq

gojq: gojq.go mod/ujsonin.go
	go build -o gojq gojq.go

clean:
	$(RM) gojq
