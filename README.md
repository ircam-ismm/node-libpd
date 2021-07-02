# `node-libpd`

> Node.js bindings for [`libpd`](https://github.com/libpd/libpd) using [`portaudio`](http://www.portaudio.com/) as audio backend.

## Install

```
npm install [--save] node-libpd
```

## Table of Content

<!-- toc -->

- [Basic Usage](#basic-usage)
- [Notes / Caveats:](#notes--caveats)
  * [Install on Mac OSX](#install-on-mac-osx)
  * [Install on Raspberry Pi](#install-on-raspberry-pi)
- [API](#api)
  * [Objects](#objects)
  * [pd : object](#pd--object)
  * [Patch : object](#patch--object)
- [Tests](#tests)
- [Todos](#todos)
- [Credits](#credits)
- [License](#license)

<!-- tocstop -->

## Basic Usage

```js
const pd = require('node-libpd');
// or if you use some transpiler such as babel
// import pd from 'node-libpd';

// init pd
const initialized = pd.init({
  numInputChannels: 0,
  numOutputChannels: 2,
  sampleRate: 48000,
  ticks: 1, // a pd block (or tick) is 64 samples, be aware that increasing this value will throttle messages (tbc)
});

// instantiate a patch
const patchPathname = path.join(process.cwd(), 'pd', 'my-patch.pd');
const patch = pd.openPatch(patchPathname);

// subscribe to messages from the patch
pd.subscribe(`${patch.$0}-output`, msg => {
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
pd.closePatch(`${patch.$0}-input`, 1234);
```

## Notes / Caveats:

- The library is meant to be used in a _Node.js_ environment, it cannot run in a browser and never will.
- The library can only be used with pd-vanilla objects, it does not, and maybe will never, support externals.
- The audio interface used is the one that is set as default system wise.
- The bindings are created with N-API, therefore v1 should work on Node.js > 10.x.x, for previous version of Node.js you should install node-libpd v0.2.6 that was created with Nan (be aware that this version won't receive support).
- The library does not support all the interface of libpd, however the most important ones should be there. The API has also been adapted at particular places to match common JS idiomatisms.

_Tested on MAC OSX 10 and Raspbian Stretch Lite version 9 (raspberry pi 3) - for other platforms, dynamic libraries for libpd and portaudio should probably be built._

### Install on Mac OSX

```
xcode-select --install
```

### Install on Raspberry Pi

```
apt-get install -y ... ???
```


## API

<!-- api -->

### Objects

<dl>
<dt><a href="#pd">pd</a> : <code>object</code></dt>
<dd><p>Singleton that represents an instance of the underlying libpd library</p>
</dd>
<dt><a href="#Patch">Patch</a> : <code>object</code></dt>
<dd><p>Object representing a patch instance.</p>
</dd>
</dl>

<a name="pd"></a>

### pd : <code>object</code>
Singleton that represents an instance of the underlying libpd library

**Kind**: global namespace  

* [pd](#pd) : <code>object</code>
    * [.currentTime](#pd.currentTime) : <code>Number</code>
    * [.init(config)](#pd.init) ⇒ <code>Boolean</code>
    * [.destroy()](#pd.destroy)
    * [.openPatch(pathname)](#pd.openPatch) ⇒ <code>Object</code>
    * [.closePatch(patch)](#pd.closePatch)
    * [.addToSearchPath(pathname)](#pd.addToSearchPath)
    * [.clearSearchPath()](#pd.clearSearchPath)
    * [.send(channel, value, [time])](#pd.send)
    * [.subscribe(channel, callback)](#pd.subscribe)
    * [.unsubscribe(channel, [callback])](#pd.unsubscribe)
    * [.writeArray(name, data, [writeLen], [offset])](#pd.writeArray) ⇒ <code>Boolean</code>
    * [.readArray(name, data, [readLen], [offset])](#pd.readArray) ⇒ <code>Boolean</code>
    * [.clearArray(name, [value])](#pd.clearArray)
    * [.arraySize(name)](#pd.arraySize) ⇒ <code>Number</code>

<a name="pd.currentTime"></a>

#### pd.currentTime : <code>Number</code>
Current audio time in seconds since `init` as been called.

**Kind**: static property of [<code>pd</code>](#pd)  
**Read only**: true  
<a name="pd.init"></a>

#### pd.init(config) ⇒ <code>Boolean</code>
Configure and initialize pd instance. You basically want to do that at the
startup of the application as the process is blocking and that it can take
 a long time to have the audio running.

**Kind**: static method of [<code>pd</code>](#pd)  
**Returns**: <code>Boolean</code> - a boolean defining if pd and portaudio have been properly
 initialized  

| Param | Type | Default | Description |
| --- | --- | --- | --- |
| config | <code>Object</code> |  |  |
| [config.numInputChannels] | <code>Number</code> | <code>1</code> | num input channels requested |
| [config.numOutputChannels] | <code>Number</code> | <code>2</code> | num output channels requested |
| [config.sampleRate] | <code>Number</code> | <code>4800</code> | requested sampleRate |
| [config.ticks] | <code>Number</code> | <code>1</code> | number of blocks (ticks) processed by pd  in one run, a pd tick is 64 samples. Be aware that this value will affect /  throttle the messages sent to and received by pd, i.e. more ticks means less  precision in the treatement of the messages. A value of 1 or 2 is generally  good enough even in constrained platforms such as the RPi. |

<a name="pd.destroy"></a>

#### pd.destroy()
Destroy the pd instance. You basically want to do that want your program
exists to clean things up, be aware the any call to the pd instance after
calliing `destroy` migth throw a SegFault error.

**Kind**: static method of [<code>pd</code>](#pd)  
<a name="pd.openPatch"></a>

#### pd.openPatch(pathname) ⇒ <code>Object</code>
Open a pd patch instance. As the same patch can be opened several times,
think of it as a kind of poly with a nice API, be careful to use patch.$0
in your patches.

**Kind**: static method of [<code>pd</code>](#pd)  
**Returns**: <code>Object</code> - - instance of the patch  

| Param | Type | Description |
| --- | --- | --- |
| pathname | <code>String</code> | absolute path to the pd patch |

<a name="pd.closePatch"></a>

#### pd.closePatch(patch)
Close a pd patch instance.

**Kind**: static method of [<code>pd</code>](#pd)  

| Param | Type | Description |
| --- | --- | --- |
| patch | [<code>Patch</code>](#Patch) | a patch instance as retrived by `openPatch` |

<a name="pd.addToSearchPath"></a>

#### pd.addToSearchPath(pathname)
Add a directory to the pd search paths, for loading libraries etc.

**Kind**: static method of [<code>pd</code>](#pd)  

| Param | Type | Description |
| --- | --- | --- |
| pathname | <code>String</code> | absolute path to the directory |

<a name="pd.clearSearchPath"></a>

#### pd.clearSearchPath()
Clear the pd search path

**Kind**: static method of [<code>pd</code>](#pd)  
<a name="pd.send"></a>

#### pd.send(channel, value, [time])
Send a named message to the pd backend

**Kind**: static method of [<code>pd</code>](#pd)  

| Param | Type | Default | Description |
| --- | --- | --- | --- |
| channel | <code>String</code> |  | name of the corresponding `receive` box in the patch  the avoid conflict a good practice is the prepend the channel name with `patch.$0` |
| value | <code>Any</code> |  | payload of the message, the corresponding mapping is  made with pd types: Number -> float, String -> symbol, Array -> list  (all value that neither Number nor String are ignored), else -> bang |
| [time] | <code>Number</code> | <code></code> | audio time at which the message should be  sent. If null or < currentTime, is sent as fast as possible. (@tbc messages  are processed at pd control rate). |

<a name="pd.subscribe"></a>

#### pd.subscribe(channel, callback)
Subscribe to named events send by a pd patch

**Kind**: static method of [<code>pd</code>](#pd)  

| Param | Type | Description |
| --- | --- | --- |
| channel | <code>String</code> | channel name corresponding to the pd send name |
| callback | <code>function</code> | callback to execute when an event is received |

<a name="pd.unsubscribe"></a>

#### pd.unsubscribe(channel, [callback])
Unsubscribe to named events send by a pd patch

**Kind**: static method of [<code>pd</code>](#pd)  

| Param | Type | Default | Description |
| --- | --- | --- | --- |
| channel | <code>String</code> |  | channel name corresponding to the pd send name |
| [callback] | <code>function</code> | <code></code> | callback that should stop receive event.  If null, all callbacks of the channel are removed |

<a name="pd.writeArray"></a>

#### pd.writeArray(name, data, [writeLen], [offset]) ⇒ <code>Boolean</code>
Write values into a pd array. Be carefull with the size of the pd arrays
(default to 100) in your patches.

**Kind**: static method of [<code>pd</code>](#pd)  
**Returns**: <code>Boolean</code> - true if the operation succeed, false otherwise  

| Param | Type | Default | Description |
| --- | --- | --- | --- |
| name | <code>Name</code> |  | name of the pd array |
| data | <code>Float32Array</code> |  | Float32Array containing the data to be written  into the pd array. |
| [writeLen] | <code>Number</code> | <code>data.length</code> | @todo confirm behavior |
| [offset] | <code>Number</code> | <code>0</code> | @todo confirm behavior |

<a name="pd.readArray"></a>

#### pd.readArray(name, data, [readLen], [offset]) ⇒ <code>Boolean</code>
Read values into a pd array.

**Kind**: static method of [<code>pd</code>](#pd)  
**Returns**: <code>Boolean</code> - true if the operation succeed, false otherwise  

| Param | Type | Default | Description |
| --- | --- | --- | --- |
| name | <code>Name</code> |  | name of the pd array |
| data | <code>Float32Array</code> |  | Float32Array to populate from pd array values |
| [readLen] | <code>Number</code> | <code>data.length</code> | @todo confirm behavior |
| [offset] | <code>Number</code> | <code>0</code> | @todo confirm behavior |

<a name="pd.clearArray"></a>

#### pd.clearArray(name, [value])
Fill a pd array with a given value.

**Kind**: static method of [<code>pd</code>](#pd)  

| Param | Type | Default | Description |
| --- | --- | --- | --- |
| name | <code>Name</code> |  | name of the pd array |
| [value] | <code>Number</code> | <code>0</code> | value used to fill the pd array |

<a name="pd.arraySize"></a>

#### pd.arraySize(name) ⇒ <code>Number</code>
Retrieve the size of a pd array.

**Kind**: static method of [<code>pd</code>](#pd)  
**Returns**: <code>Number</code> - size of the array  

| Param | Type | Description |
| --- | --- | --- |
| name | <code>Name</code> | name of the pd array |

<a name="Patch"></a>

### Patch : <code>object</code>
Object representing a patch instance.

**Kind**: global namespace  

* [Patch](#Patch) : <code>object</code>
    * [.$0](#Patch.$0) : <code>Number</code>
    * [.isValid](#Patch.isValid) : <code>Boolean</code>
    * [.filename](#Patch.filename) : <code>String</code>
    * [.path](#Patch.path) : <code>String</code>

<a name="Patch.$0"></a>

#### Patch.$0 : <code>Number</code>
Id of the patch. You should use this value to communicate with a given patch
in send and receive channel.

**Kind**: static property of [<code>Patch</code>](#Patch)  
**Read only**: true  
<a name="Patch.isValid"></a>

#### Patch.isValid : <code>Boolean</code>
Validity of the patch instance. False typically means that the given filename
does not point to a valid pd patch, or that the patch has been closed.

**Kind**: static property of [<code>Patch</code>](#Patch)  
**Read only**: true  
<a name="Patch.filename"></a>

#### Patch.filename : <code>String</code>
Name of the the pd patch file

**Kind**: static property of [<code>Patch</code>](#Patch)  
**Read only**: true  
<a name="Patch.path"></a>

#### Patch.path : <code>String</code>
Directory of the pd patch file

**Kind**: static property of [<code>Patch</code>](#Patch)  
**Read only**: true  

<!-- apistop -->


## Tests

To run the tests

```sh
# cf. test/index.js
npm run test
```

## Todos

- list devices and choose device, is a portaudio problem
- midi event ? is there some real-world use case
- support pd externals, if only possible...
- rebuild portaudio without jack on linux

## Credits

The library has received support from the Ircam's project BeCoMe.

## License

BSD-3-Clause
