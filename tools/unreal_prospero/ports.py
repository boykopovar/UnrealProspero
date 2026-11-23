import json
from pathlib import Path
import shutil

from .engine import versions
from .install import digest
from .process import require_tool, run
from .sources import checkout


class Ports:
    def __init__(self, engine, sdk, dependencies, work, output, cmake, jobs):
        self.engine = engine
        self.third_party = engine / 'Engine/Source/ThirdParty'
        self.sdk = sdk
        self.dependencies = dependencies
        self.work = work
        self.output = output
        self.cmake = cmake
        self.jobs = jobs
        self.root = Path(__file__).resolve().parents[2]
        self.platform = output / 'Engine/Platforms/UnrealProspero'

    def build(self, name, prefix, variables, component=None, configuration=None):
        directory = self.work / name
        source = self.root / 'Engine/Platforms/UnrealProspero/Build' / (configuration or name)
        arguments = [self.cmake, '-S', source, '-B', directory, '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_TOOLCHAIN_FILE=' + (self.sdk / 'lib/cmake/AnyDevKit/toolchains/ps5.cmake').as_posix(), '-DAnyDevKit_DIR=' + (self.sdk / 'lib/cmake/AnyDevKit').as_posix(), '-DADK_DEPENDENCY_ROOT=' + (self.dependencies / 'ps5').as_posix(), '-DADK_SYSROOT=' + (self.dependencies / 'ps5/sysroot').as_posix(), '-DCMAKE_INSTALL_PREFIX=' + prefix.as_posix()]
        arguments += ['-D' + key + '=' + (value.as_posix() if isinstance(value, Path) else str(value)) for key, value in variables.items()]
        run(arguments, self.root, self.work, self.work / ('logs/' + name + '-configure.log'))
        run([self.cmake, '--build', directory, '--parallel', self.jobs], self.root, self.work, self.work / ('logs/' + name + '-build.log'))
        install = [self.cmake, '--install', directory]
        if component is not None:
            install += ['--component', component]
        run(install, self.root, self.work, self.work / ('logs/' + name + '-install.log'))

    def third_party_ports(self):
        self.base_ports()
        self.font_ports()
        self.media_ports()

    def base_ports(self):
        source = self.third_party
        destination = self.platform / 'Source/ThirdParty'
        zlib = source / 'zlib/1.3'
        png = source / 'libPNG/libPNG-1.6.44'
        self.build('Blake3', destination / 'BLAKE3/1.3.1', {'BLAKE3_SOURCE_DIRECTORY': source / 'BLAKE3/1.3.1/c'})
        self.move_library(destination / 'BLAKE3/1.3.1/lib/libBLAKE3.a', destination / 'BLAKE3/1.3.1/lib/AnyPS5/libBLAKE3.a')
        self.build('Zlib', destination / 'zlib/1.3', {'ZLIB_SOURCE_DIRECTORY': zlib})
        self.move_library(destination / 'zlib/1.3/lib/libz.a', destination / 'zlib/1.3/lib/AnyPS5/libz.a')
        self.build('LibPng', destination / 'libPNG/libPNG-1.6.44', {'PNG_SOURCE_DIRECTORY': png, 'ZLIB_SOURCE_DIRECTORY': zlib})
        self.move_library(destination / 'libPNG/libPNG-1.6.44/lib/libpng.a', destination / 'libPNG/libPNG-1.6.44/lib/AnyPS5/libpng.a')
        self.build('ICU', destination / 'ICU/icu4c-64_1', {'ICU_SOURCE_DIRECTORY': source / 'ICU/icu4c-64_1/source'})
        self.move_library(destination / 'ICU/icu4c-64_1/lib/libicu.a', destination / 'ICU/icu4c-64_1/lib/AnyPS5/libicu.a')

    def font_ports(self):
        source = self.third_party
        destination = self.platform / 'Source/ThirdParty'
        zlib = source / 'zlib/1.3'
        png = source / 'libPNG/libPNG-1.6.44'
        freetype = destination / 'FreeType2/FreeType2-2.14.1/AnyPS5'
        self.build('FreeType', freetype, {'FREETYPE_SOURCE_DIRECTORY': source / 'FreeType2/FreeType2-2.14.1', 'PNG_PNG_INCLUDE_DIR': png, 'PNG_LIBRARY': destination / 'libPNG/libPNG-1.6.44/lib/AnyPS5/libpng.a', 'ZLIB_INCLUDE_DIR': zlib, 'ZLIB_LIBRARY': destination / 'zlib/1.3/lib/AnyPS5/libz.a'})
        self.build('HarfBuzz', destination / 'HarfBuzz/harfbuzz-2.4.0', {'HARFBUZZ_SOURCE_DIRECTORY': source / 'HarfBuzz/harfbuzz-2.4.0/src', 'FREETYPE_INCLUDE_DIRECTORY': freetype / 'include/freetype2', 'ICU_INCLUDE_DIRECTORY': source / 'ICU/icu4c-64_1/include'})
        self.move_library(destination / 'HarfBuzz/harfbuzz-2.4.0/lib/libharfbuzz.a', destination / 'HarfBuzz/harfbuzz-2.4.0/lib/AnyPS5/libharfbuzz.a')

    def media_ports(self):
        source = self.third_party
        destination = self.platform / 'Source/ThirdParty'
        self.build('MikkTSpace', destination / 'MikkTSpace', {'MIKKTSPACE_SOURCE_DIRECTORY': source / 'MikkTSpace'})
        self.move_library(destination / 'MikkTSpace/lib/libMikkTSpace.a', destination / 'MikkTSpace/lib/AnyPS5/libMikkTSpace.a')
        self.build('Opus', destination / 'libOpus/opus-1.1/AnyPS5', {'OPUS_SOURCE_DIRECTORY': source / 'libOpus/opus-1.1'})
        audio = {'OGG_SOURCE_DIRECTORY': source / 'Ogg/libogg-1.2.2', 'VORBIS_SOURCE_DIRECTORY': source / 'Vorbis/libvorbis-1.3.2'}
        self.build('OggVorbis', destination / 'Ogg/libogg-1.2.2/AnyPS5', audio, 'Ogg')
        run([self.cmake, '--install', self.work / 'OggVorbis', '--prefix', destination / 'Vorbis/libvorbis-1.3.2/AnyPS5', '--component', 'Vorbis'], self.root, self.work, self.work / 'logs/Vorbis-install.log')

    @staticmethod
    def move_library(source, destination):
        destination.parent.mkdir(parents=True, exist_ok=True)
        source.replace(destination)

    def runtime_ports(self):
        runtime = self.engine / 'Engine/Source/Runtime'
        destination = self.platform / 'Source/Runtime'
        self.build('BinkAudio', destination / 'BinkAudioDecoder/SDK/BinkAudio', {'BINK_AUDIO_SDK_DIRECTORY': runtime / 'BinkAudioDecoder/SDK/BinkAudio', 'BINK_AUDIO_PLATFORM': 'AnyPS5'})
        self.build('RadAudio', destination / 'RadAudioCodec/SDK', {'RAD_AUDIO_SDK_DIRECTORY': runtime / 'RadAudioCodec/SDK', 'RAD_AUDIO_PLATFORM': 'AnyPS5'})
        for component, source, target in (
            ('core', runtime / 'OodleDataCompression/Sdks/2.9.16', destination / 'OodleDataCompression/Sdks/2.9.16/lib/AnyPS5'),
            ('network', self.engine / 'Engine/Plugins/Compression/OodleNetwork/Sdks/2.9.16', self.platform / 'Plugins/Compression/OodleNetwork/Sdks/2.9.16/lib/AnyPS5')
        ):
            prefix = 'oo2core' if component == 'core' else 'oo2net'
            for debug in (False, True):
                name = 'Oodle-' + component + ('-debug' if debug else '')
                archive = source / 'lib/Linux' / ('lib' + prefix + 'linux64' + ('_dbg' if debug else '') + '.a')
                self.build(name, self.work / (name + '-install'), {'OODLE_ARCHIVE': archive, 'OODLE_COMPONENT': component, 'OODLE_DEBUG': 'ON' if debug else 'OFF', 'LIBC_IMPORT_LIBRARY': self.dependencies / 'ps5/imports/libc.prx'}, configuration='Oodle')
                self.move_library(self.work / (name + '-install/lib') / ('lib' + prefix + ('_debug' if debug else '') + '.a'), target / ('lib' + prefix + ('_debug' if debug else '') + '.a'))

    def network_ports(self):
        specification = versions()
        nghttp2 = checkout(specification['nghttp2'], self.work / 'sources/nghttp2', [], self.work)
        self.build('Nghttp2', self.platform / 'Source/ThirdParty/nghttp2/AnyPS5', {'NGHTTP2_SOURCE_DIRECTORY': nghttp2})
        openssl = checkout(specification['openssl'], self.work / 'sources/openssl', [], self.work)
        prepared = self.work / 'openssl-source'
        shutil.copytree(openssl, prepared, ignore=shutil.ignore_patterns('.git'))
        script = self.root / 'Engine/Platforms/UnrealProspero/Build/OpenSsl/Build.sh'
        arguments = [require_tool('bash'), script, prepared, self.work / 'openssl-build', self.platform / 'Source/ThirdParty/OpenSSL/AnyPS5', self.dependencies / 'ps5/sysroot', self.dependencies / 'include/anyps5', self.dependencies / 'ps5/anycxx', self.jobs]
        run([value.as_posix() if isinstance(value, Path) else value for value in arguments], self.root, self.work, self.work / 'logs/OpenSsl-build.log')

    def write_manifest(self):
        files = {path.relative_to(self.output).as_posix(): digest(path) for path in sorted(self.output.rglob('*')) if path.is_file() and (path.suffix == '.a' or 'include' in path.relative_to(self.output).parts)}
        if not files:
            raise ValueError('No PS5 libraries were produced')
        (self.output / 'PortsManifest.json').write_text(json.dumps({'schemaVersion': 1, 'files': files}, indent=2) + '\n', encoding='utf-8')
