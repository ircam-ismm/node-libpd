const nodelibpd = require('bindings')('nodelibpd');
const path = require('path');

/**
 * @namespace pd
 */
/**
 *
 *
 */
/**
 * @method send
 * @memberof pd
 *
 * send a named message to the pd backend
 *
 * @param {String} channel - name of the corresponding `receive` box in the patch
 *  the avoid conflict a good practice is the prepend the channel name with `patch.$0`
 * @param {Any} value - payload of the message, the corresponding mapping is
 *  made with pd types: Number -> float, String -> symbol, Array -> list
 *  (all value that neither Number nor String are ignored), else -> bang
 * @param {Number} [time=null] - audio time at which the message should be
 *  sent. If null or < currentTime, is sent as fast as possible. (@tbc messages
 *  are processed at pd control rate).
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
