# node-libpd

> `nodejs` wrapper around `libpd` and `portaudio`, 

## install

```
npm install [--save] b-ma/node-libpd
```

can be quite long...

#### tested on

- Mac OSX 10.11
- next step: raspbian lite

## Next Steps

- test on raspberry pi
- clock (currentTime, etc...)
- `pd.clear()` to stop background processes

## Todos

#### tests

- find a proper way to organize tests

#### audio

- allow to discover available devices and configuration for input and output 
  => (maybe this should be done in a separate module)
- more generally expose more audio configuration options
- stream audio to javascript
- do something when numInputChannels and numOutputChannels are 0

#### messaging - I/O

- implement array API
- refactor messaging struct (cf `pd_msg_t`)
  + use more specialized structs and `dynamic_pointer_cast`
  + use `const` and references as in PdReceiver callbacks
- implement midi in/out messaging

#### patches

- re-enable `addToSearchPath` and `clearSearchPath`

#### misc

- move `LockedQueue` implementation in `.cpp` file
- stop the whole pd and portaudio instances
- install babel thing to rewrite the index.js in es6
  + would be fancy to have an `index.mjs` and an `index.js`
- add a `verbose` options to `init`
- properly handle errors using : `Nan::ThrowError("...");`
- deployement
  + find what we can do with `libs` folder for `libpd`
  + how to deploy binaries ?

### Caveats

`receive` - These are asynchronous by nature, should not be used for precise scheduling.

`unsubscribe` - As messages can already be in the queue when unsubscribing, some messages can be triggered after the call of this method.

## Building

```
$ npm i
$ node-gyp configure
$ node-gyp build
# if something chaned in bindings.gyp
$ node-gyp rebuild
```

You can confirm everything built correctly by [running the test suite](#to-run-tests).

### Run tests:

```
$ npm test
```

## Resources

`libpd` usage example (patch + test): 
https://github.com/libpd/libpd/tree/master/samples/cpp/pdtest

Node/Nan tutorial
https://nodejs.org/api/addons.html#addons_wrapping_c_objects
https://nodeaddons.com/book/ 
https://medium.com/netscape/tutorial-building-native-c-modules-for-node-js-using-nan-part-1-755b07389c7c

libuv book: 
https://nikhilm.github.io/uvbook/threads.html

Debug with lldb: 
https://medium.com/@ccnokes/how-to-debug-native-node-addons-in-mac-osx-66f69f81afcb

