OUT = buck-out

all: test

$(OUT)/gen/river/river:
	buck build //river:river

.PHONY: test
test: $(OUT)/gen/river/river
	./$< test/hello.js

.PHONY: clean
clean:
	rm -rf ./$(OUT)
