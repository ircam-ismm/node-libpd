{
  "targets": [
    {
      "target_name": "nodelibpd",
      "sources": [
        "nodelibpd.cpp",
        "./src/BackgroundProcess.cpp",
        "./src/NodePd.cpp",
        "./src/PaWrapper.cpp",
        "./src/PdReceiver.cpp",
        "./src/PdWrapper.cpp",
        "./src/types.cpp",
      ],
      "include_dirs" : [
        "<!(node -e \"require('nan')\")",

        # include deps headers
        "./portaudio/include",
        "./libpd/include",

        # needed by PdBase.h itself
        "./libpd/include/libpd",
        "./libpd/include/libpd/util",
			],

      # https://github.com/nodejs/node-gyp/issues/1296
      # 'library_dirs': [
      #   '<(module_root_dir)/deps/lib',
      #   '<(module_root_dir)/libs', # fix lib pd inconsistency
      # ],

      "cflags": [
        "-std=c++11",
        # "-stdlib=libc++"
      ],
      'cflags!': [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],

      'conditions': [
        [
          'OS=="mac"', {
            'xcode_settings': {
              'MACOSX_DEPLOYMENT_TARGET': '10.7',
              'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
              'OTHER_CPLUSPLUSFLAGS' : [
                '-std=c++11','-stdlib=libc++',
                '-Wno-sign-compare',
              ],
              'OTHER_LDFLAGS': [
                '-stdlib=libc++',"-Wl,-rpath,<@(module_root_dir)/build/Release"
              ],
              # 'GCC_ENABLE_CPP_RTTI': 'YES',
            },
            'link_settings': {
              'libraries': [
                '<@(module_root_dir)/build/Release/libportaudio.dylib',
                '<@(module_root_dir)/build/Release/libpd.dylib',
              ],
            },
            "copies": [
              {
                "destination": "build/Release/",
                "files": [
                  "<!@(ls -1 portaudio/lib/libportaudio.dylib)"
                ]
              },
              {
                "destination": "build/Release/",
                "files": [
                  "<!@(ls -1 libpd/lib/libpd.dylib)"
                ]
              }
            ]
          }
        ],

        [
          'OS=="linux"', {
            "conditions": [
              ['target_arch=="arm"', {
                "link_settings": {
                  "libraries": [
                    "<@(module_root_dir)/build/Release/libportaudio.so.2",
                    "<@(module_root_dir)/build/Release/libpd.so",
                  ],
                  "ldflags": [
                    "-L<@(module_root_dir)/build/Release",
                    "-Wl,-rpath,<@(module_root_dir)/build/Release",
                    '-Wno-sign-compare',
                  ]
                },
                "copies": [
                  {
                    "destination": "build/Release/",
                    "files": [
                      "<@(module_root_dir)/portaudio/lib-armhf/libportaudio.so.2"
                    ]
                  },
                  {
                    "destination": "build/Release/",
                    "files": [
                      "<@(module_root_dir)/libpd/lib-armhf/libpd.so"
                    ]
                  }
                ]
              }]
            ]
          }
        ]

      ] # end conditions

    },
  ],
}
