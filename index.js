const nodelibpd = require('bindings')('nodelibpd');
const path = require('path');

/**
 * Singleton that represents an instance of the underlying libpd library
 * @namespace pd
 */
/**
 * Current audio time in seconds since `init` as been called.
 *
 * @member currentTime
 * @type {Number}
 * @memberof pd
 */
/**
 * Configure and initialize pd instance. You basically want to do that at the
 * startup of the application as the process is blocking and that it can take
 *  a long time to have the audio running.
 *
 * @function init
 * @memberof pd
 * @param {Object} config
 * @param {Number} [config.numInputChannels=1] - num input channels requested
 * @param {Number} [config.numOutputChannels=2] - num output channels requested
 * @param {Number} [config.sampleRate=4800] - requested sampleRate
 * @param {Number} [config.ticks=1] - number of blocks (ticks) processed by pd
 *  in one run, a pd tick is 64 samples. Be aware that this value will affect
 *  the messages sent to and received by pd, i.e. more ticks means less precision
 *  in the treatement of the messages.
 * @return {Boolean} a boolean defining if pd and portaudio have been properly
 *  initialized
 */
/**
 * Destroy the pd instance. You basically want to do that want your program
 * exists to clean things up, be aware the any call to the pd instance after
 * calliing `destroy` migth throw a SegFault error.
 *
 * @function destroy
 * @memberof pd
 */
/**
 * Open a pd patch instance. As the same patch can be opened several times,
 * think of it as a kind of poly with a nice API, be careful to use patch.$0
 * in your patches.
 *
 * @function openPatch
 * @memberof pd
 * @param {pathname} - absolute path to the pd patch
 * @return {Patch} - instance of the patch
 */
/**
 * Close a pd patch instance.
 *
 * @function closePatch
 * @memberof pd
 * @param {Patch} patch - a patch instance as retrived by `openPatch`
 */
/**
 * Add a directory to the pd search paths, for loading libraries etc.
 *
 * @function addToSearchPath
 * @memberof pd
 * @param {String} pathname - absolute path to the directory
 */
/**
 * Clear the pd search path
 *
 * @function clearSearchPath
 * @memberof pd
 */
/**
 * Send a named message to the pd backend
 * @function send
 * @memberof pd
 * @param {String} channel - name of the corresponding `receive` box in the patch
 *  the avoid conflict a good practice is the prepend the channel name with `patch.$0`
 * @param {Any} value - payload of the message, the corresponding mapping is
 *  made with pd types: Number -> float, String -> symbol, Array -> list
 *  (all value that neither Number nor String are ignored), else -> bang
 * @param {Number} [time=null] - audio time at which the message should be
 *  sent. If null or < currentTime, is sent as fast as possible. (@tbc messages
 *  are processed at pd control rate).
 */
/**
 * Subscribe to named events send by a pd patch
 *
 * @function subscribe
 * @memberof pd
 * @param {String} channel - channel name corresponding to the pd send name
 * @param {Function} callback - callback to execute when an event is received
 */
/**
 * Unsubscribe to named events send by a pd patch
 *
 * @function unsubscribe
 * @memberof pd
 * @param {String} channel - channel name corresponding to the pd send name
 * @param {Function} [callback=null] - callback that should stop receive event.
 *  If null, all callbacks of the channel are removed
 */
/**
 * Write values into a pd array. Be carefull with the size of the pd arrays
 * (default to 100) in your patches.
 *
 * @function writeArray
 * @memberof pd
 * @param {Name} name - name of the pd array
 * @param {Float32Array} data - Float32Array containing the data to be written
 *  into the pd array.
 * @param {Number} [writeLen=data.length] - @todo confirm behavior
 * @param {Number} offset - @todo confirm behavior
 * @return {Boolean} true if the operation succeed, false otherwise
 */
/**
 * Read values into a pd array.
 *
 * @function readArray
 * @memberof pd
 * @param {Name} name - name of the pd array
 * @param {Float32Array} data - Float32Array to populate from pd array values
 * @param {Number} [readLen=data.length] - @todo confirm behavior
 * @param {Number} offset - @todo confirm behavior
 * @return {Boolean} true if the operation succeed, false otherwise
 */
/**
 * Fill a pd array with a given value.
 *
 * @function clearArray
 * @memberof pd
 * @param {Name} name - name of the pd array
 * @param {Number} value - value used to fill the pd array
 */
/**
 * Fill a pd array with a given value.
 *
 * @function arraySize
 * @memberof pd
 * @param {Name} name - name of the pd array
 * @return {Number} size of the array
 */

/**
 * Object representing a patch instance.
 * @namespace Patch
 */
/**
 * Id of the patch. You should use this value to communicate with a given patch
 * in send and receive channel.
 * @member $0
 * @type {Number}
 * @memberof Patch
 */
/**
 * Tells you if the patch is valid, for example is Valid is false is the patch
 * @member isValid
 * @type {Boolean}
 * @memberof Patch
 */
/**
 * Name of the the pd patch file
 * @member filename
 * @type {String}
 * @memberof Patch
 */
/**
 * Directory of the pd patch file
 * @member path
 * @type {Number}
 * @memberof Patch
 */


// singleton
const pd = new nodelibpd.NodePd();

let listenersChannelMap = {};

// global receive function that dispatch to subscriptions
const dispatch = function(channel, value) {
  const listeners = listenersChannelMap[channel];

  // as a message can still arrive from pd after `unsubscribe` have been
  // called because of the inter-processs queue, we have to check that
  // listeners still exists
  if (listeners && listeners.length > 0) {
    listeners.forEach(function(listener) { listener(value); });
  }
};

let initialized = false;

pd.init = (options = {}) => {
  if (!initialized) {
    initialized = pd._initialize(options, dispatch);
  }

  return initialized;
}

// allow both syntax:
// pd.openPatch(path.join(patchesPath, 'my-patch.pd')); // js friendly
// pd.openPatch('my-patch.pd', patchesPath); // pd native API for backward compatibility
pd.openPatch = function(...args) {
  if (args.length === 1) {
    const filename = path.basename(args[0]);
    const dirname = path.dirname(args[0]);
    return pd._openPatch(filename, dirname);
  } else {
    const [filename, dirname] = args;
    return pd._openPatch(filename, dirname);
  }
}

pd.subscribe = function(channel, callback) {
  if (!listenersChannelMap[channel]) {
    listenersChannelMap[channel] = [];
    pd._subscribe(channel);
  }

  listenersChannelMap[channel].push(callback);
}

pd.unsubscribe = function(channel, callback = null) {
  const listeners = listenersChannelMap[channel];

  if (Array.isArray(listeners)) {
    if (callback !== null) {
      const index = listeners.indexOf(callback);

      if (index !== -1) {
        listeners.splice(index, 1);
      }
    }

    if (callback === null || listeners.length === 0) {
      pd._unsubscribe(channel);
      delete listenersChannelMap[channel];
    }
  }
}

module.exports = pd;
