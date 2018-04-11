OUT = buck-out

all: test

# River
$(OUT)/gen/river/river:
	buck build //river:river

.PHONY: test
test: $(OUT)/gen/river/river
	./$< test/hello.js

# UV
$(OUT)/gen/uvpp/uvpp-demo:
	buck build //uvpp:uvpp-demo

.PHONY: test-uvpp
test-uvpp: $(OUT)/gen/uvpp/uvpp-demo
	./$<

# Cleanup
.PHONY: clean
clean:
	rm -rf ./$(OUT)
