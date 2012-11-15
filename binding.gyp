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
        'deps/libogg/libogg.gyp:ogg',
      ],
      'conditions': [
        ['OS=="mac"', {
          'libraries': [
            # re-export all libogg.a symbols for other node native
            # modules to be able to link to this ogg.node file
            '-Wl,-force_load,<(module_root_dir)/build/$(BUILDTYPE)/libogg.a'
          ]
        }]
      ]
    }
  ]
}
