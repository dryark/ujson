all: gojq

gojq: gojq.go
	go build -o gojq gojq.go