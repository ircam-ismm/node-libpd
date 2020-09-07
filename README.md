# `node-libpd`

> Wrapper around `libpd` and `portaudio` for Node.js

## Notes / Caveats:
- The library is meant to be used in a _Node.js_ environment, it cannot run in a browser and never will.
- The library can only be used with pd-vanilla objects, it does not support (yet) patches that import external objects.
- The library can run on `Node.js` <= 10.x.x, build fails on `Node.js` >= 12.x.x

_Tested on MAC OSX 10 and Raspbian Stretch Lite version 9 (raspberry pi 3) - for other platforms, dynamic libraries for libpd and portaudio should probably be built._

## Install

```
npm install [--save] node-libpd
```


## Usage

```js
import pd from 'node-libpd';

// init pd
const initialized = pd.init({
  numInputChannels: 0,
  numOutputChannels: 2,
  sampleRate: 48000,
  ticks: 1, // a pd block (or tick) is 64 samples, be aware that increasing this value will throttle messages
});

// 
const patchesPath = path.join(process.cwd(), 'my-patches');
const patch = pd.openPatch('my-patch.pd', patchesPath);

// subscribe to messages from the patchd
pd.subscribe(`${patch.$0}-output`, function(msg) {
  console.log(msg);
});

// send message to the patch
pd.send(`${patch.$0}-input`, 1234);

// send a scheduled message
const now = pd.currentTime; // time in sec.
// send message to the patch in two seconds from `now`, this accuracy of the
// scheduling will depend on the number of ticks defined at initialization
pd.send(`${patch.$0}-input`, 1234, now + 2);

// close the patch
pd.close(`${patch.$0}-input`, 1234);
```

### Tests:

```
# cf. test/index.js
$ npm run test
```

## Todos

- port to NAPI to support current LTS and future version of Node.js
- support pd externals

## Credits

The library has received support from the Ircam's project BeCoMe.

## License

BSD-3-Clause
