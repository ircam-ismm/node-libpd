{
  "targets": [
    {
      "target_name": "nodelibpd",
      "sources": [
        "nodelibpd.cc",
        "./src/NodePd.cc",
        "./src/types.cc",
        "./src/PaWrapper.cc",
        "./src/PdReceiver.cc",
        "./src/PdWrapper.cc",
        "./src/BackgroundProcess.c",
      ],
      "include_dirs" : [
        "<!@(node -p \"require('node-addon-api').include\")",
        # # include deps headers
        "./portaudio/include",
        "./libpd/include",

        # # needed by PdBase.h itself
        "./libpd/include/libpd",
        "./libpd/include/libpd/util",
			],

      # https://github.com/nodejs/node-gyp/issues/1296
      # 'library_dirs': [
      #   '<(module_root_dir)/deps/lib',
      #   '<(module_root_dir)/libs', # fix lib pd inconsistency
      # ],

      "defines": ["NAPI_CPP_EXCEPTIONS"],
      # "cflags": [
      #   "-std=c++11",
      #   # "-stdlib=libc++"
      # ],
      'cflags!': [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],

      'conditions': [
        [
          'OS=="mac"', {
            'xcode_settings': {
              'GCC_ENABLE_CPP_RTTI': 'YES',
              'MACOSX_DEPLOYMENT_TARGET': '10.7',
              'OTHER_CPLUSPLUSFLAGS': [
                '-std=c++11',
                '-stdlib=libc++',
                '-fexceptions',
              ],
              'OTHER_LDFLAGS': [
                "-Wl,-rpath,<@(module_root_dir)/build/Release"
              ]
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
              }],

            ['target_arch=="x64"', {        
                "link_settings": {
                  "libraries": [
                    "<@(module_root_dir)/build/Release/libportaudio.so.2.0.0",
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
                      "<@(module_root_dir)/portaudio/lib-x64/libportaudio.so.2.0.0"
                    ]
                  },
                  {
                    "destination": "build/Release/",
                    "files": [
                      "<@(module_root_dir)/libpd/lib-x64/libpd.so"
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
