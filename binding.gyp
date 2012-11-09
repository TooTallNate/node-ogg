{
  'targets': [
    {
      'target_name': 'binding',
      'sources': [
        'src/binding.cc',
      ],
      'dependencies': [
        'deps/libogg/libogg.gyp:ogg',
      ],
    }
  ]
}
