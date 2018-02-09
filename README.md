# node-libpd

> `nodejs` wrapper around `libpd` and `portaudio`

Tested on MAC OSX 10.11 and Raspbian Stretch Lite version 9 (raspberry pi 3)

_note: for other platforms, dynamic libraries for libpd and portaudio should probably be built._

## Install

```
npm install [--save] b-ma/node-libpd
```

### Run tests:

```
$ npm test
```

## Raspberry pi tests

test ">>>>> poly like": 

- on-board audio card: needs at least 32 ticks
- HifiBerry dac+ - works with 1 tick

_@todo_

- test other sound cards (including one with input)
- 

## Next Steps

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

#### messaging - I/O

- implement array API
- refactor messaging struct (cf `pd_msg_t`)
  + use more specialized structs and `dynamic_pointer_cast`
  + use `const` and references as in PdReceiver callbacks
- implement midi in/out messaging

#### patches

- re-enable `addToSearchPath` and `clearSearchPath`

#### misc

- stop the whole pd and portaudio instances
- make `init` asynchronous to fix the race condition between js and worker threads (initialization can be quite long (> 100ms on mac OSX)). `init` should return a `Promise` => current workaround is to block the `init` method until `currentTime != 0`
- move `LockedQueue` implementation in `.cpp` file
- install babel thing to rewrite the index.js in es6
  + would be fancy to have an `index.mjs` and an `index.js`
- add a `verbose` options to `init`
- properly handle errors using : `Nan::ThrowError("...");`

## Caveats

- `receive` - This method is asynchronous by nature, should not be used for precise scheduling.
- `send` is processed on each block (`blockSize * ticks`) -> looks like this a pd behavior

## Resources

`libpd` usage example (patch + test): 
- https://github.com/libpd/libpd/tree/master/samples/cpp/pdtest  

Node/Nan tutorial
- https://nodejs.org/api/addons.html#addons_wrapping_c_objects  
- https://nodeaddons.com/book/  
- https://medium.com/netscape/tutorial-building-native-c-modules-for-node-js-using-nan-part-1-755b07389c7c  

libuv book: 
- https://nikhilm.github.io/uvbook/threads.html

Debug with lldb: 
- https://medium.com/@ccnokes/how-to-debug-native-node-addons-in-mac-osx-66f69f81afcb



## Problems on raspberry pi

- port audio do not seem to find to find jack server


