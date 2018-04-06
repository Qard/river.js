OUT = buck-out

all: test

# River
$(OUT)/gen/river/river:
	buck build //river:river

.PHONY: test
test: $(OUT)/gen/river/river
	./$< test/hello.js

# UV
$(OUT)/gen/uv/uv-demo:
	buck build //uv:uv-demo

.PHONY: test-uv
test-uv: $(OUT)/gen/uv/uv-demo
	./$<

# Cleanup
.PHONY: clean
clean:
	rm -rf ./$(OUT)
