{
  'targets': [
    {
      'target_name': 'ogg',
      'sources': [
        'src/binding.cc',
      ],
      'dependencies': [
        'deps/libogg/libogg.gyp:ogg',
      ],
    }
  ]
}
