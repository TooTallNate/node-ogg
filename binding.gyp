{
  'targets': [
    {
      'type': 'shared_library',
      'product_extension': 'node',
      'defines': [ 'BUILDING_NODE_EXTENSION' ],
      'target_name': 'ogg',
      'sources': [
        'src/binding.cc',
      ],
      'dependencies': [
        'deps/libogg/libogg.gyp:libogg',
      ],
      'conditions': [
        ['OS=="mac"', {
          # re-export all libogg.a symbols for other node native
          # modules to be able to link to this ogg.node file. on linux/solaris
          # this happens because of the "shared_library" type...
          'libraries': [
            '-Wl,-force_load,<(module_root_dir)/build/$(BUILDTYPE)/ogg.a'
          ],
        }],
        ['OS=="win"', {
          # on Windows we have to use the ogg.def file to ensure that all the
          # libogg symbols get re-exported
          'msvs_settings': {
            'VCLinkerTool': {
              'ModuleDefinitionFile': '<(module_root_dir)\\deps\\libogg\\win32\\ogg.def',
            },
          },
        }],
      ],
    },
  ],
}
