from conans import ConanFile, tools
import platform


class ModuleConan(ConanFile):
    name = "QtConanExample"
    description = "An example for Qt with Conan"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake", "cmake_find_package_multi", "cmake_paths"
    default_options = {
        "openscenegraph:shared": True,
        "osgearth:shared": True,
        "boost:shared": True,
        "openscenegraph:opengl_profile": "gl3",
    }

    def configure(self):
        del self.settings.compiler.cppstd

    def requirements(self):
        self.requires("zlib/1.2.13", override=True)
        self.requires("freetype/2.13.0", override=True)
        self.requires("libpng/1.6.37", override=True)
        self.requires("libjpeg/9e", override=True)
        self.requires("zstd/1.5.5", override=True)
        self.requires("libdeflate/1.18", override=True)
        self.requires("libwebp/1.3.1", override=True)
        self.requires("openssl/3.1.4", override=True)
        self.requires("sqlite3/3.42.0", override=True)
        self.requires("gtest/1.14.0")
        self.requires("openscenegraph/3.6.5")
        self.requires("osgearth/3.3")
        self.requires("boost/1.83.0")
        self.requires("libkml/1.3.0")

        
    def imports(self):
        self.copy("*.dll", "./bin", "bin")
        self.copy("*.so", "./bin", "bin")