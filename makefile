
test:
	conan create . ckristen/master --build=missing

export:
	conan export . ckristen/master && conan test test_package tdlib/1.3.0@ckristen/master 

upload:
	conan upload tdlib/1.3.0@ckristen/master -r ckristen --all