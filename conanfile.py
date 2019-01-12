from conans import ConanFile, CMake, tools
import os


class TdlibConan(ConanFile):
    name = "tdlib"
    version = "1.3.0"
    license = "Boost Software License 1.0"
    url = "https://github.com/veyroter/conan-tdlib"
    homepage = "https://github.com/tdlib/td"
    description = "conan tdlib package"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"
    folder_version = version
    exports_sources = "td-%s%s*" % (folder_version, os.sep)
    requires = "OpenSSL/1.0.2n@conan/stable", "zlib/1.2.11@conan/stable"

    def configure(self):
        self.options["OpenSSL"].shared = False
        self.options["zlib"].shared = False

    def build(self):
        print("self.dir_bld(): %s" % self.dir_bld())
        print("self.dir_src(): %s" % self.dir_src())
        cmake = CMake(self)
        cmake.definitions["CMAKE_VERBOSE_MAKEFILE"] = True
        cmake.definitions["OPENSSL_ROOT_DIR"] = self.deps_cpp_info["OpenSSL"].rootpath
        cmake.definitions["ZLIB_ROOT"] = self.deps_cpp_info["zlib"].rootpath
        tools.mkdir(self.dir_bld())
        cmake.configure(source_folder=self.dir_src(), cache_build_folder=self.dir_bld())
        cmake.build(build_dir=self.dir_bld())

    def package(self):
        cmake = CMake(self)
        cmake.configure(source_folder=self.dir_src(), cache_build_folder=self.dir_bld())
        cmake.install()

    def package_info(self):
        # TODO: windows and osx

        # reversed order of dependency found in file lib/cmake/Td/TdTargets.cmake 
        # in install directory
        if self.settings.os != "Linux":
            self.cpp_info.libs = [
                "tdjson_static",
                "tdjson_private",
                "tdclient",
                "tdcore",
                "tddb",
                "tdsqlite",
                "tdnet",
                "tdactor",
                "tdutils",
                "Mswsock",
                "Normaliz",
            ]
        else:
            self.cpp_info.libs = [
                "libtdjson_static.a",
                "libtdjson_private.a",
                "libtdclient.a",
                "libtdcore.a",
                "libtddb.a",
                "libtdsqlite.a",
                "libtdnet.a",
                "libtdactor.a",
                "libtdutils.a",
                "m",
                "dl",
                "pthread",
            ]
            self.cpp_info.cppflags = [
                "-D_GLIBCXX_USE_CXX11_ABI=1",
            ]
        # if self.settings.os == "Linux":
        #     self.cpp_info.libs.extend(["dl", "m"])

    def dir_src(self):
        try:
            return self.src_full_path
        except:
            self.src_full_path = "%s%std-%s" % (self.source_folder, os.sep, self.folder_version)
            return self.src_full_path

    def dir_bld(self):
        try:
            return self.build_full_path
        except:
            self.build_full_path = "%s%sbuild" % (self.dir_src(), os.sep)
            return self.build_full_path
