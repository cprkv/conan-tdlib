name = tdlib
version = 1.3.0

test:
	conan create . ckristen/master --build=missing

export:
	conan export . ckristen/master && conan test test_package $(name)/$(version)@ckristen/master 

upload:
	conan upload $(name)/$(version)@ckristen/master -r ckristen --all