from conans import ConanFile, CMake
import os

class TestConanPackage(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"

    def build(self):
        cmake = CMake(self)
        cmake.definitions["CMAKE_VERBOSE_MAKEFILE"] = True
        cmake.definitions["CMAKE_CXX_STANDARD"] = 14
        cmake.definitions["CMAKE_CXX_STANDARD_REQUIRED"] = True
        cmake.definitions["CMAKE_CXX_EXTENSIONS"] = False
        cmake.configure()
        cmake.build()

    def imports(self):
        self.copy("*.dll", dst="bin", src="bin")
        self.copy("*.dylib*", dst="bin", src="lib")

    def test(self):
        os.chdir("bin")
        self.run(".%stest_package" % os.sep)