from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMakeToolchain, CMakeDeps, CMake


class MauiRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    def requirements(self):
        self.requires("expat/2.6.2")
        self.tool_requires("doxygen/1.9.2")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.cache_variables["DBUS_SERVICE"] = True
        tc.generate()

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
