# `node-libpd`

> Node.js binding for [`lib-pd`](https://github.com/libpd/libpd) that uses [`portaudio`](http://www.portaudio.com/) as audio backend.

## Table of Content

<!-- toc -->

- [Notes / Caveats:](#notes--caveats)
- [Install](#install)
  * [Install on Mac OSX](#install-on-mac-osx)
  * [Install on Raspberry Pi](#install-on-raspberry-pi)
- [Usage](#usage)
- [API](#api)
  * [Objects](#objects)
  * [Functions](#functions)
  * [pd : object](#pd--object)
  * [pd.send](#pdsend)
- [Tests](#tests)
- [Todos](#todos)
- [Credits](#credits)
- [License](#license)

<!-- tocstop -->

## Notes / Caveats:

- The library is meant to be used in a _Node.js_ environment, it cannot run in a browser and never will.
- The library can only be used with pd-vanilla objects, it does not, and maybe will never, support externals.
- The bindings are created with N-API, therefore v1 should work on Node.js > 10.x.x, for previous version of Node.js you should install node-libpd v0.2.6 that was created with Nan (be aware that this version won't receive support).

_Tested on MAC OSX 10 and Raspbian Stretch Lite version 9 (raspberry pi 3) - for other platforms, dynamic libraries for libpd and portaudio should probably be built._

## Install

```
npm install [--save] node-libpd
```

### Install on Mac OSX

```
xcode-select --install
```

### Install on Raspberry Pi

```
apt-get install -y ... ???
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

## API

<!-- api -->

### Objects

<dl>
<dt><a href="#pd">pd</a> : <code>object</code></dt>
<dd></dd>
</dl>

### Functions

<dl>
<dt><a href="#pd.send

send a named message to the pd backend">pd.send

send a named message to the pd backend(channel, value, [time])</a></dt>
<dd></dd>
</dl>

<a name="pd"></a>

### pd : <code>object</code>
**Kind**: global namespace  
<a name="pd.send

send a named message to the pd backend"></a>

### pd.send

send a named message to the pd backend(channel, value, [time])
**Kind**: global function  

| Param | Type | Default | Description |
| --- | --- | --- | --- |
| channel | <code>String</code> |  | name of the corresponding `receive` box in the patch  the avoid conflict a good practice is the prepend the channel name with `patch.$0` |
| value | <code>Any</code> |  | payload of the message, the corresponding mapping is  made with pd types: Number -> float, String -> symbol, Array -> list  (all value that neither Number nor String are ignored), else -> bang |
| [time] | <code>Number</code> | <code></code> | audio time at which the message should be  sent. If null or < currentTime, is sent as fast as possible. (@tbc messages  are processed at pd control rate). |


<!-- apistop -->

## Tests

To run the tests

```sh
# cf. test/index.js
npm run test
```

## Todos

- support pd externals (see if only it's possible...)
- rebuild libpd without jack

## Credits

The library has received support from the Ircam's project BeCoMe.

## License

BSD-3-Clause
