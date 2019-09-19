import os
from conans import CMake, ConanFile
from conans.errors import ConanException


class X16Emu(ConanFile):
    name = 'x16-emulator'
    version = 'master'
    exports = ('LICENSE', )

    requires = ('sdl2/2.0.10@bincrafters/stable', )

    # Adding the path of x16docs and x16rom is not good conan practice.
    options = {
        'ym2151': [True, False, ],
        'release': [True, False, ],
        'x16docs_path': 'ANY',
        'x16rom_path': 'ANY',
    }
    default_options = {
        'ym2151': True,
        'release': False,
        'x16docs_path': None,
        'x16rom_path': None,
    }

    settings = 'os', 'arch', 'compiler', 'build_type',
    generators = 'cmake_paths',

    def configure(self):
        del self.settings.compiler.libcxx

    def config_options(self):
        if not os.path.isdir(str(self.options.x16docs_path)):
            raise ConanException('Need a valid x16docs_path path')
        if not  os.path.isdir(str(self.options.x16rom_path)):
            raise ConanException('Need a valid x16rom_path path')

    def build(self):
        cmake = CMake(self)
        cmake.definitions['WITH_YM2151'] = self.options.ym2151
        cmake.definitions['X16EMU_RELEASE'] = self.options.release
        if self.options.x16docs_path:
            cmake.definitions['X16DOCS_PATH'] = self.options.x16docs_path
        if self.options.x16rom_path:
            cmake.definitions['X16ROM_PATH'] = self.options.x16rom_path
        cmake.configure(args=['-DCMAKE_PROJECT_x16emu_INCLUDE={}'.format(
            os.path.join(self.build_folder, "conan_paths.cmake")),])
        cmake.build()

    def package_id(self):
        del self.info.options.x16docs_path
        del self.info.options.x16rom_path

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.env_info.PATH.append(os.path.join(self.package_folder, 'bin'))
